/*
 * Grid.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef PAGED_GRID_HPP_
#define PAGED_GRID_HPP_

#include "Vector3.hpp"
#include "MurmurHash2.hpp"

#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <fstream>
#include <assert.h>
#include <inttypes.h>

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

	//	bool contains(const size3_t index) {
	//		if (index.x < origin.x)
	//			return false;
	//		if (index.y < origin.y)
	//			return false;
	//		if (index.z < origin.z)
	//			return false;
	//
	//		if (index.x >= (origin.x + size))
	//			return false;
	//		if (index.y >= (origin.y + size))
	//			return false;
	//		if (index.z >= (origin.z + size))
	//			return false;
	//
	//		return true;
	//	}

	element_t &get(const size3_t index) {
#ifdef DEBUG
		assert(index.x > orgin.x)
		assert(index.x < orgin.x + size)
		assert(index.y > orgin.y)
		assert(index.Y < orgin.y + size)
		assert(index.z > orgin.z)
		assert(index.z < orgin.z + size)
#endif
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
#if DEBUG
		std::cerr << "[BinaryPageIO] load page " << page->origin << ", size: "
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
		size_t page_byte_size = page->size * page->size * page->size * sizeof(element_t);
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
			//			std::cerr << "[BinaryPageIO] could not open " << filename << std::endl;
			return false;
		}

		in.seekg(0, std::ios::end);
		if (in.bad()) {
			//			std::cerr << "[BinaryPageIO] bad seek " << filename << std::endl;
			return false;
		}

		std::fstream::pos_type needed = page->size * page->size * page->size * fileSize * fileSize * fileSize * sizeof(element_t);
		if (in.tellg() != needed) {
			//			std::cerr << "baad size " << filename << std::endl;

			return false;
		}

		return true;

	}

	void reserveFile(const std::string &filename, page_t *page) {
		size_t needed =  page->size * page->size * page->size * fileSize * fileSize * fileSize;
		std::cerr << "[BinaryPageIO] reserve " << filename << std::endl;
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
	size_t maxCount;

	LeastAccessPagingStrategy() :
		maxCount(1) {
	}

	void loaded(page_t *page) {
		pages[page] = maxCount / 10;
	}

	void cleared(page_t *page) {
		pages[page] = 0;
	}

	void accessed(page_t *page) {
		size_t &v = pages[page];
		v += 1;
		if (v > maxCount)
			maxCount = v;
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

//static size_t hash3(const size3_t &v) {
//	return MurmurHash2(&v, sizeof(size3_t), 875685);
//}

template<typename ELEMENT>
class PagedGrid {
public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	typedef typename std::map<size3_t, page_t *> container_t;
	typedef typename container_t::iterator iterator_t;

	class Visitor {
	public:
		virtual void visit(PagedGrid<element_t> &grid, size_t x, size_t y,
				size_t z, element_t &value) = 0;
	};

	PagingStrategy<element_t> *strategy;
	PageIO<element_t> *io;
	container_t pages;
	page_t *lastPage;
	size_t size;
	size_t pageCount;
	size_t pageSize;
	size_t pageMisses;

	PagedGrid() :
		lastPage(0), pageCount(1), pageMisses(0) {
	}

	PagedGrid(const size_t &grid_size, size_t page_size) :
		lastPage(0), pageCount(1), pageMisses(0) {
		create(grid_size, page_size);
	}

	~PagedGrid() {
		clear();
	}

	void create(const size_t &grid_size, size_t page_size) {
		size = grid_size;
		pageSize = page_size;
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
					pages.erase(hash3(page->origin));
					delete page;
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
		size3_t orig = toOrigin(index);

		if (lastPage && (lastPage->origin == orig))
			return lastPage;

		iterator_t i = pages.find(orig);
		if (i != pages.end()) {
			return i->second;
		}

		// check if page is loaded
		page_t *page = 0;
		if (pages.size() < pageCount) {
			page = new page_t;
		} else {
			// ask the paging strategy which page we should replace
			pageMisses++;
			page = strategy->which();

			// save and clear old page
			io->savePage(page);
			strategy->cleared(page);
			pages.erase(page->origin);
		}

		// load new page
		page->origin = orig;
		page->size = pageSize;
		io->loadPage(page);
		strategy->loaded(page);
		lastPage = page;

		pages[orig] = page;

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
		if (index.x > size)
			index.x = size;
		if (index.y > size)
			index.y = size;
		if (index.z > size)
			index.z = size;

		return index;
	}

	void clear() {
		iterator_t i = pages.begin();
		iterator_t end = pages.end();
		while (i != end) {
			delete (i->second);
			i++;
		}
		pages.clear();
	}

	void flush() {
		iterator_t i = pages.begin();
		iterator_t end = pages.end();
		while (i != end) {
			io->savePage(i->second);
			i++;
		}
	}

	size_t getActivePageCount() {
		return pages.size();
	}

	size_t getPageMisses() {
		return pageMisses;
	}

	void acceptXYZ(Visitor &v) {
		for (size_t iX = 0; iX < size; iX++) {
			for (size_t iY = 0; iY < size; iY++) {
				for (size_t iZ = 0; iZ < size; iZ++) {
					v.visit(*this, iX, iY, iZ, getReadWrite(iX, iY, iZ));
				}
			}
		}
	}

	void acceptZYX(Visitor &v) {
		for (size_t iZ = 0; iZ < size; iZ++) {
			for (size_t iY = 0; iY < size; iY++) {
				for (size_t iX = 0; iX < size; iX++) {
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

	void accept(Visitor &v, const size3_t &l, const size3_t &u) {
		size3_t upper = clamp(u);

		size3_t pmin = l / pageSize;
		size3_t pmax = upper / pageSize;

		//size_t visits = 0;
		for (size_t pX = pmin.x; pX <= pmax.x; pX++) {
			size_t lx = std::max(l.x, pX * pageSize);
			size_t ux = std::min(upper.x, pX * pageSize + pageSize);
			for (size_t pY = pmin.y; pY <= pmax.y; pY++) {
				size_t ly = std::max(l.y, pY * pageSize);
				size_t uy = std::min(upper.y, pY * pageSize + pageSize);
				for (size_t pZ = pmin.z; pZ <= pmax.z; pZ++) {
					size_t lz = std::max(l.z, pZ * pageSize);
					size_t uz = std::min(upper.z, pZ * pageSize + pageSize);

					page_t *page = getPage(size3_t(lx, ly, lz));
					strategy->accessed(page);

					//visits += ((uz - lz) * (uy - ly) * (ux - lx));
					size3_t index;
					for (size_t iZ = lz; iZ < uz; iZ++) {
						index.z = iZ;
						for (size_t iY = ly; iY < uy; iY++) {
							index.y = iY;
							for (size_t iX = lx; iX < ux; iX++) {
								index.x = iX;
								v.visit(*this, iX, iY, iZ, page->get(index));
							}
						}
					}

				}
			}
		}
		//
		//		if (visits > 0) {
		//			std::cerr << pmin << std::endl;
		//			std::cerr << pmax << std::endl;
		//
		//			std::cerr << visits << std::endl;
		//			std::cerr << std::endl;
		//		}
	}

};

#endif /* PAGED_GRID_HPP_ */
