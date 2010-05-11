/*
 * Grid.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef PAGED_GRID_HPP_
#define PAGED_GRID_HPP_

#include "Vector3.hpp"

#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <fstream>

typedef Vector3<size_t> size3_t;

template<typename ELEMENT>
class Page {
public:
	typedef ELEMENT element_t;

	size3_t origin;
	size_t size;
	std::vector<element_t> elements;
	bool dirty;
	Page() :
		size(0) {

	}

	bool contains(const size3_t index) {
		if (index.x < origin.x)
			return false;
		if (index.y < origin.y)
			return false;
		if (index.z < origin.z)
			return false;

		if (index.x >= (origin.x + size))
			return false;
		if (index.y >= (origin.y + size))
			return false;
		if (index.z >= (origin.z + size))
			return false;

		return true;
	}

	element_t &get(const size3_t index) {
		return elements[(index.x - origin.x) * size * size + (index.y
				- origin.y) * size + (index.z - origin.z)];
	}
};

template<typename ELEMENT>
class PageIO {
public:
	typedef Page<ELEMENT> page_t;
	virtual void loadPage(page_t *page) = 0;
	virtual void savePage(page_t *page) = 0;
};

template<typename ELEMENT>
class SimpleTextPageIO: public PageIO<ELEMENT> {
public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	std::string prefix;
	element_t defaultValue;
	bool readOnly;

	void loadPage(page_t *page) {
		page->elements.resize(page->size * page->size * page->size);
		std::ifstream in(createFilename(page).c_str());
		if (in.good()) {
			for (size_t i = 0; i < page->elements.size() && in; i++) {
				in >> page->elements[i];
			}
		} else {
			for (size_t i = 0; i < page->elements.size(); i++) {
				page->elements[i] = defaultValue;
			}
		}
		page->dirty = false;

	}
	void savePage(page_t *page) {
		if (readOnly)
			return;
		std::ofstream out(createFilename(page).c_str(), std::ios::trunc);
		for (size_t i = 0; i < page->elements.size() && out; i++) {
			out << page->elements[i] << std::endl;
		}
		page->dirty = false;
	}
private:
	std::string createFilename(page_t *page) {
		std::stringstream sstr;
		sstr << prefix << "-" << page->size << "-" << page->origin.x << "_"
				<< page->origin.y << "_" << page->origin.z << ".txt";
		return sstr.str();
	}
};

template<typename ELEMENT>
class BinaryPageIO: public PageIO<ELEMENT> {
public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	std::string prefix;
	element_t defaultValue;
	bool readOnly;
	size_t loadedPages;
	size_t savedPages;
	size_t fileSize;
	bool forceDump;

	BinaryPageIO() :
		readOnly(false), loadedPages(0), savedPages(0), fileSize(1), forceDump(
				false) {

	}
	void loadPage(page_t *page) {
#ifdef DEBUG
		std::cout << "[BinaryPageIO] load page " << page->origin << ", size: "
		<< page->size << std::endl;
#endif
		page->elements.resize(page->size * page->size * page->size);
		std::string filename = createFilename(page);
		page->dirty = false;
		if (checkFile(filename, page)) {
			std::ifstream in(filename.c_str(), std::ios::binary);
			in.seekg(offset(page), std::ios::beg);
			in.read((char *) &page->elements.at(0), sizeof(element_t)
					* page->elements.size());
		} else {
			for (size_t i = 0; i < page->elements.size(); i++) {
				page->elements[i] = defaultValue;
			}
			if (forceDump)
				page->dirty = true;
		}

		loadedPages += 1;
	}

	size_t offset(page_t *page) {
		size_t ox = (page->origin.x / page->size) % fileSize;
		size_t oy = (page->origin.y / page->size) % fileSize;
		size_t oz = (page->origin.z / page->size) % fileSize;
		size_t page_byte_size = std::pow(page->size, 3) * sizeof(element_t);
		return page_byte_size * (ox * fileSize * fileSize + oy * fileSize + oz);
	}

	void savePage(page_t *page) {
		savedPages += 1;
		if (readOnly)
			return;

		if (page->dirty == false)
			return;

#ifdef DEBUG
		std::cout << "[BinaryPageIO] save page " << page->origin << std::endl;
#endif
		std::string filename = createFilename(page);

		// make sure the file is big enough
		if (checkFile(filename, page) == false)
			reserveFile(filename, page);

		std::fstream out(filename.c_str(), std::ios::in | std::ios::out
				| std::ios::binary);
		out.seekp(offset(page), std::ios::beg);
		out.write((const char *) &page->elements.at(0), sizeof(element_t)
				* page->elements.size());

		page->dirty = false;
	}

private:

	std::string createFilename(page_t *page) {
		size_t ox = (page->origin.x / page->size) / fileSize;
		size_t oy = (page->origin.y / page->size) / fileSize;
		size_t oz = (page->origin.z / page->size) / fileSize;
		std::stringstream sstr;
		sstr << prefix << "-" << fileSize << "-" << ox << "_" << oy << "_"
				<< oz << ".raw";
		return sstr.str();
	}

	bool checkFile(const std::string &filename, page_t *page) {
		std::ifstream in(filename.c_str(), std::ios::binary);
		if (in.good() == false) {
			return false;
		}

		in.seekg(0, std::ios::end);
		if (in.bad()) {
			return false;
		}

		std::fstream::pos_type needed = std::pow(page->size, 3) * std::pow(
				fileSize, 3) * sizeof(element_t);
		if (in.tellg() != needed) {
			return false;
		}

		return true;

	}

	void reserveFile(const std::string &filename, page_t *page) {
		size_t needed = std::pow(page->size, 3) * std::pow(fileSize, 3);

		std::ofstream out(filename.c_str(), std::ios::trunc | std::ios::binary);
		for (size_t i = 0; i < needed; i++) {
			out.write((const char *) &defaultValue, sizeof(defaultValue));
		}
		out.close();

	}
};

template<typename ELEMENT>
class PagingStrategy {
public:
	typedef Page<ELEMENT> page_t;
	virtual void loaded(page_t *page) = 0;
	virtual void cleared(page_t *page) = 0;
	virtual void accessed(page_t *page) = 0;
	virtual page_t *which() = 0;
};

template<typename ELEMENT>
class LastAccessPagingStrategy: public PagingStrategy<ELEMENT> {
public:
	typedef Page<ELEMENT> page_t;
	std::list<page_t *> pages;
	void loaded(page_t *page) {
		pages.push_back(page);
	}
	void cleared(page_t *page) {
		pages.remove(page);
	}
	void accessed(page_t *page) {
		pages.remove(page);
		pages.push_front(page);
	}
	page_t *which() {
		return pages.back();
	}
};

template<typename ELEMENT>
class LeastAccessPagingStrategy: public PagingStrategy<ELEMENT> {
public:
	typedef Page<ELEMENT> page_t;
	typedef std::map<page_t *, size_t> map_t;
	typedef typename map_t::iterator iterator_t;
	map_t pages;
	void loaded(page_t *page) {
		size_t min = -1, max = 0;
		for (iterator_t i = pages.begin(); i != pages.end(); i++) {
			i->second /= 2;
			if (i->second < min)
				min = i->second;
			else if (i->second > max)
				max = i->second;
		}

		pages[page] = (max - min) / 2;
	}
	void cleared(page_t *page) {
		pages[page] = 0;
	}
	void accessed(page_t *page) {
		pages[page] += 1;
	}
	page_t *which() {
		size_t count = -1;
		page_t *page = 0;
		for (iterator_t i = pages.begin(); i != pages.end(); i++)
			if (i->second < count) {
				page = i->first;
				count = i->second;
			}
		return page;
	}
};

template<typename ELEMENT>
class PagedGrid {
public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	typedef typename std::list<page_t *> list_t;
	typedef typename list_t::iterator list_iterator_t;

	class Visitor {
	public:
		virtual void visit(PagedGrid<element_t> &grid, size_t x, size_t y,
				size_t z, element_t &value) = 0;
	};

	PagingStrategy<element_t> *strategy;
	PageIO<element_t> *io;
	std::list<page_t *> pages;
	size3_t size;
	size_t pageCount;
	size_t pageSize;
	size3_t origin;

	PagedGrid(size3_t grid_size, size_t page_size) :
		size(grid_size), pageCount(1), pageSize(page_size) {

	}

	~PagedGrid() {
		while (pages.size()) {
			delete pages.back();
			pages.pop_back();
		}
	}

	void setStrategy(PagingStrategy<element_t> *strategy) {
		this->strategy = strategy;
	}

	void setIO(PageIO<element_t> *io) {
		this->io = io;
	}

	void setPageCount(size_t count) {
		if (count < pageCount) {
			size_t clearing = this->pageCount - count;
			for (size_t i = 0; i < clearing; i++) {
				page_t *page = strategy->which();
				if (page) {
					io->savePage(page);
					strategy->cleared(page);
					delete page;
					pages.remove(page);
				}
			}
		}
		this->pageCount = count;
	}

	element_t &getReadWrite(const size3_t &index) {
		page_t *page = getPage(index);
		strategy->accessed(page);
		page->dirty = true;
		return page->get(index);
	}

	const element_t &getReadOnly(const size3_t &index) {
		page_t *page = getPage(index);
		strategy->accessed(page);
		return page->get(index);
	}

	page_t *getPage(const size3_t &index) {
		// check if page is loaded
		list_iterator_t i = pages.begin();
		list_iterator_t end = pages.end();
		while (i != end) {
			if ((*i)->contains(index))
				return (*i);
			i++;
		}
		page_t *page = 0;

		if (pages.size() < pageCount) {
			page = new page_t;
			pages.push_back(page);
		} else {
			// ask the paging strategy which page we should replace
			page = strategy->which();

			// save and clear old page
			io->savePage(page);
			strategy->cleared(page);
		}

		// load new page
		page->origin = toOrigin(index);
		page->size = pageSize;
		io->loadPage(page);
		strategy->loaded(page);

		return page;
	}

	size3_t toOrigin(const size3_t &index) {
		size3_t origin;
		origin.x = size_t(index.x / pageSize) * pageSize;
		origin.y = size_t(index.y / pageSize) * pageSize;
		origin.z = size_t(index.z / pageSize) * pageSize;
		return origin;
	}

	size3_t clamp(const size3_t &i) {
		size3_t index = i;
		if (index.x > size.x)
			index.x = size.x;
		if (index.y > size.y)
			index.y = size.y;
		if (index.z > size.z)
			index.z = size.z;

		return index;
	}

	void flush() {
		list_iterator_t i = pages.begin();
		list_iterator_t end = pages.end();
		while (i != end) {
			io->savePage(*i);
			i++;
		}
	}

	void acceptXYZ(Visitor &v) {
		for (size_t iX = 0; iX < size.x; iX++) {
			for (size_t iY = 0; iY < size.y; iY++) {
				for (size_t iZ = 0; iZ < size.z; iZ++) {
					v.visit(*this, iX, iY, iZ, getReadWrite(iX, iY, iZ));
				}
			}
		}
	}

	size_t getActivePageCount() {
		return pages.size();
	}

	void acceptZYX(Visitor &v) {
		for (size_t iZ = 0; iZ < size.z; iZ++) {
			for (size_t iY = 0; iY < size.y; iY++) {
				for (size_t iX = 0; iX < size.x; iX++) {
					v.visit(*this, iX, iY, iZ, getReadWrite(iX, iY, iZ));
				}
			}
		}
	}

	void acceptZYX(Visitor &v, const size3_t &l, const size3_t &u) {
		size3_t index;
		size3_t upper = clamp(u);
		for (size_t iZ = l.z; iZ < upper.z; iZ++) {
			index.z = iZ;
			for (size_t iY = l.y; iY < upper.y; iY++) {
				index.y = iY;
				for (size_t iX = l.x; iX < upper.x; iX++) {
					index.x = iX;
					v.visit(*this, iX, iY, iZ, getReadWrite(index));
				}
			}
		}
	}

};

#endif /* PAGED_GRID_HPP_ */
