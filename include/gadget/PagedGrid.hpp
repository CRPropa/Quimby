/*
 * Grid.hpp
 *
 *  Created on: 02.02.2010
 *      Author: gmueller
 */

#ifndef PAGED_GRID_HPP_
#define PAGED_GRID_HPP_

#include "MurmurHash2.hpp"
#include "Index3.hpp"

#include <algorithm>
#include <vector>
#include <list>
#include <map>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <assert.h>
#include <stdexcept>

/// Single Page containing the elements.
template<typename ELEMENT>
class Page {
public:
	typedef ELEMENT element_t;

	Index3 origin;
	element_t *elements;
	bool dirty;
	//	time_t accessTime;
	//	uint32_t accessCount;
	//	uint32_t locks;

	Page *strategyNext;
	Page *strategyPrev;

	Page();
	void reset();
	element_t &get(const Index3 &index, uint32_t size);
};

template<typename ELEMENT>
inline Page<ELEMENT>::Page() {
	reset();
}

template<typename ELEMENT>
inline void Page<ELEMENT>::reset() {
	dirty = false;
	//	accessTime = 0;
	//	accessCount = 0;
	//	locks = 0;
	elements = 0;
	strategyNext = 0;
	strategyPrev = 0;
}

template<typename ELEMENT>
inline typename Page<ELEMENT>::element_t &Page<ELEMENT>::get(
		const Index3 &index, uint32_t size) {
	assert(index.x >= origin.x);
	assert(index.x < origin.x + size);
	assert(index.y >= origin.y);
	assert(index.y < origin.y + size);
	assert(index.z >= origin.z);
	assert(index.z < origin.z + size);

	return elements[(index.x - origin.x) + (index.y - origin.y) * size
			+ (index.z - origin.z) * size * size];
}

template<typename ELEMENT>
class PageIO {
protected:
	uint32_t elementsPerPage;
	uint32_t elementsPerPage3;

public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	virtual void setElementsPerPage(uint32_t pageSize) {
		this->elementsPerPage = pageSize;
		this->elementsPerPage3 = pageSize * pageSize * pageSize;
	}
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
		size_t count = page->size * page->size * page->size;
		std::ifstream in(createFilename(page).c_str());
		if (in.good()) {
			for (size_t i = 0; i < count && in; i++) {
				in >> page->elements[i];
			}
		} else {
			for (size_t i = 0; i < count; i++) {
				page->elements[i] = defaultValue;
			}
		}
		page->dirty = false;
	}

	void savePage(page_t *page) {
		if (readOnly)
			return;
		size_t count = page->size * page->size * page->size;
		std::ofstream out(createFilename(page).c_str(), std::ios::trunc);
		for (size_t i = 0; i < count && out; i++) {
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
private:
	std::string prefix;
	element_t defaultValue;
	bool readOnly;
	size_t loadedPages;
	size_t savedPages;

	size_t elementsPerFile;
	size_t elementsPerFile3;

	size_t pagesPerFile;
	size_t pagesPerFile3;

	bool overwrite;

	bool forceDump;

	bool perPage;

public:
	BinaryPageIO();

	void loadPage(page_t *page);

	void savePage(page_t *page);

	void setPrefix(const std::string &prefix);

	/// set number of elements per file per axis
	void setElementsPerFile(size_t elemetsPerFile);

	void setOverwrite(bool overwrite);
	void setForceDump(bool forceDump);
	void setReadOnly(bool readOnly);
	void setDefaultValue(const element_t &defaultValue);

	size_t getLoadedPages();
	size_t getSavedPages();
	void setElementsPerPage(uint32_t pageSize);

private:
	size_t offset(page_t *page, const Index3 idx) {
		Index3 offset = idx + (page->origin % elementsPerFile);
		return sizeof(element_t) * (offset.x + offset.y * elementsPerFile
				+ offset.z * elementsPerFile * elementsPerFile);
	}

	size_t page_offset(page_t *page) {
		Index3 o = (page->origin % elementsPerFile) / this->elementsPerPage;
		size_t page_byte_size = this->elementsPerPage3 * sizeof(element_t);
		return page_byte_size * (o.x * pagesPerFile * pagesPerFile + o.y
				* pagesPerFile + o.z);
	}

	std::string createFilename(page_t *page) {
		Index3 o = page->origin / elementsPerFile;
		std::stringstream sstr;
		sstr << prefix;
		if (perPage) {
			sstr << "-" << pagesPerFile;
		}
		sstr << "-" << o.x << "-" << o.y << "-" << o.z << ".raw";
		return sstr.str();
	}

	bool checkFile(const std::string &filename) {
		std::ifstream in(filename.c_str(), std::ios::binary);
		if (in.good() == false) {
			if (readOnly)
				throw std::runtime_error("[BinaryPageIO] file not found:" + filename);
			return false;
		}

		std::fstream::pos_type needed = elementsPerFile3 * sizeof(element_t);

		in.seekg(0, std::ios::end);
		if (in.bad()) {
            if (readOnly)
                throw std::runtime_error("[BinaryPageIO] error sseking in file.");
			return false;
		}

		std::fstream::pos_type pos = in.tellg();
		if (pos != needed) {
            if (readOnly)
                throw std::runtime_error("[BinaryPageIO] file has wrong size.");
			return false;
		}

		return true;

	}

	void reserveFile(const std::string &filename) {
		std::ofstream out(filename.c_str(), std::ios::trunc | std::ios::binary);
		for (size_t i = 0; i < elementsPerFile3; i++) {
			out.write((const char *) &defaultValue, sizeof(element_t));
		}
		out.close();
	}

};

template<typename ELEMENT>
BinaryPageIO<ELEMENT>::BinaryPageIO() :
	readOnly(false), loadedPages(0), savedPages(0), elementsPerFile(1),
			elementsPerFile3(1), pagesPerFile(1), pagesPerFile3(1), overwrite(
					false), forceDump(false), perPage(true) {

}

template<typename ELEMENT>
void BinaryPageIO<ELEMENT>::loadPage(page_t *page) {
#if DEBUG
	std::cout << "[BinaryPageIO] load page " << page->origin << ", size: "
	<< this->elementsPerPage << std::endl;
#endif
	std::string filename = createFilename(page);
	page->dirty = false;
	bool fileCheck = checkFile(filename);
	if (fileCheck && overwrite == false) {
		std::ifstream in(filename.c_str(), std::ios::binary);
		if (perPage) {
			in.seekg(page_offset(page), std::ios::beg);
			in.read((char *) page->elements, sizeof(element_t)
					* this->elementsPerPage3);
		} else {
			Index3 index;
			for (index.z = 0; index.z < this->elementsPerPage; index.z++) {
				for (index.y = 0; index.y < this->elementsPerPage; index.y++) {
					index.x = 0;
					in.seekg(offset(page, index), std::ios::beg);
					in.read((char *) &page->get(index + page->origin,
							this->elementsPerPage), this->elementsPerPage
							* sizeof(element_t));
				}
			}
		}
	} else if (readOnly == false) {
		for (size_t i = 0; i < this->elementsPerPage3; i++) {
			page->elements[i] = defaultValue;
		}
		if (forceDump)
			page->dirty = true;
	} else {
		throw std::runtime_error("[BinaryPageIO] file not found.");
	}

	loadedPages += 1;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::savePage(page_t *page) {
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
	if (checkFile(filename) == false)
		reserveFile(filename);

	std::fstream out(filename.c_str(), std::ios::in | std::ios::out
			| std::ios::binary);

	if (perPage) {
		out.seekp(page_offset(page), std::ios::beg);
		out.write((const char *) page->elements, sizeof(element_t)
				* this->elementsPerPage3);
	} else {
		Index3 index;
		for (index.z = 0; index.z < this->elementsPerPage; index.z++) {
			for (index.y = 0; index.y < this->elementsPerPage; index.y++) {
				index.x = 0;
				out.seekp(offset(page, index), std::ios::beg);
				out.write((const char *) &page->get(index + page->origin,
						this->elementsPerPage), this->elementsPerPage
						* sizeof(element_t));

			}
		}
	}
	page->dirty = false;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setPrefix(const std::string &prefix) {
	this->prefix = prefix;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setElementsPerFile(size_t elementsPerFile) {
	this->elementsPerFile = elementsPerFile;
	elementsPerFile3 = elementsPerFile * elementsPerFile * elementsPerFile;
	pagesPerFile = elementsPerFile / this->elementsPerPage;
	pagesPerFile3 = pagesPerFile * pagesPerFile * pagesPerFile;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setElementsPerPage(uint32_t elementsPerPage) {
	PageIO<ELEMENT>::setElementsPerPage(elementsPerPage);
	pagesPerFile = elementsPerFile / elementsPerPage;
	pagesPerFile3 = pagesPerFile * pagesPerFile * pagesPerFile;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setForceDump(bool forceDump) {
	this->forceDump = forceDump;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setReadOnly(bool readOnly) {
	this->readOnly = readOnly;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setOverwrite(bool overwrite) {
	this->overwrite = overwrite;
}

template<typename ELEMENT>
inline void BinaryPageIO<ELEMENT>::setDefaultValue(
		const element_t &defaultValue) {
	this->defaultValue = defaultValue;
}

template<typename ELEMENT>
inline size_t BinaryPageIO<ELEMENT>::getLoadedPages() {
	return loadedPages;
}

template<typename ELEMENT>
inline size_t BinaryPageIO<ELEMENT>::getSavedPages() {
	return savedPages;
}

template<typename ELEMENT>
class PagingStrategy {
public:
	typedef Page<ELEMENT> page_t;
	virtual void loaded(page_t *page) = 0;
	virtual void cleared(page_t *page) = 0;
	virtual void accessed(page_t *page) = 0;
	virtual page_t *which(std::vector<page_t> &pages) = 0;
};

template<typename ELEMENT>
class LastAccessPagingStrategy: public PagingStrategy<ELEMENT> {
public:
	typedef Page<ELEMENT> page_t;
private:
	page_t *first, *last;
public:
	LastAccessPagingStrategy() :
		first(0), last(0) {
	}

	void loaded(page_t *page) {
		if (first == 0 || last == 0) {
			page->strategyPrev = 0;
			page->strategyNext = 0;
			last = page;
			first = page;
		} else {
			// insert at end
			page->strategyPrev = last;
			page->strategyPrev->strategyNext = page;
			page->strategyNext = 0;
			last = page;
		}
	}

	void cleared(page_t *page) {
		if (page == first)
			first = page->strategyNext;

		if (page == last)
			last = page->strategyPrev;

		// remove element
		if (page->strategyPrev) {
			page->strategyPrev->strategyNext = page->strategyNext;
		}
		if (page->strategyNext) {
			page->strategyNext->strategyPrev = page->strategyPrev;
		}
	}

	void accessed(page_t *page) {
		//move accessed to last
		if (page == last)
			return;

		if (page == first)
			first = page->strategyNext;

		// remove element
		if (page->strategyPrev) {
			page->strategyPrev->strategyNext = page->strategyNext;
		}
		if (page->strategyNext) {
			page->strategyNext->strategyPrev = page->strategyPrev;
		}

		// insert at end
		page->strategyPrev = last;
		page->strategyPrev->strategyNext = page;
		page->strategyNext = 0;
		last = page;
	}

	page_t *which(std::vector<page_t> &pages) {
		// TODO: find first not locked
		return first;
	}
};
/*
 template<typename ELEMENT>
 class LeastAccessPagingStrategy: public PagingStrategy<ELEMENT> {
 page_t *least;
 public:
 typedef Page<ELEMENT> page_t;

 LeastAccessPagingStrategy() :
 least(0) {
 }

 void loaded(page_t *page) {
 }

 void cleared(page_t *page) {
 }

 void accessed(page_t *page) {
 }

 page_t *which(std::vector<page_t> &pages) {
 size_t count = -1;
 page_t *page = 0;
 typename std::vector<page_t>::iterator i;
 for (i = pages.begin(); i != pages.end(); i++) {
 page_t &p = *i;
 if (p.elements != 0 && p.accessCount < count) {
 page = &p;
 count = p.accessCount;
 }
 }
 return page;
 }
 };
 */
//static size_t hash3(const Index3 &v) {
//	return MurmurHash2(&v, sizeof(Index3), 875685);
//}

template<typename ELEMENT>
class PagedGrid {
public:
	typedef ELEMENT element_t;
	typedef Page<element_t> page_t;
	typedef typename std::vector<page_t> page_container_t;
	typedef typename std::vector<element_t> element_container_t;
	typedef typename std::map<Index3, page_t *> page_index_t;
	typedef typename std::map<Index3, page_t *>::iterator
			page_index_iterator_t;

	class Visitor {
	public:
		virtual void visit(PagedGrid<element_t> &grid, size_t x, size_t y,
				size_t z, element_t &value) = 0;
	};
protected:
	page_t *lastPage;
	size_t pageSize;
	PageIO<element_t> *io;
	/// size of the grid
	size_t size;

	PagingStrategy<element_t> *strategy;
	page_container_t pages;
	element_container_t elements;
	page_index_t pageIndex;
	size_t pageMisses;

	void pageAccept(Page<element_t> *page, Visitor &v, const Index3 &l,
			const Index3 &u);

	page_t *getPage(const Index3 &index);
	page_t *getEmptyPage();
public:

	PagedGrid();
	PagedGrid(const size_t &grid_size, size_t page_size);
	~PagedGrid();

	/// set the number of elemets per axis
	void setSize(size_t size);

	void setStrategy(PagingStrategy<element_t> *strategy);
	void setIO(PageIO<element_t> *io);

	/// set the number of elemets per axis per page
	void setPageSize(uint32_t pageSize);
	/// set the number of pages held in memory
	void setPageCount(size_t count);

	element_t &getReadWrite(const Index3 &index);

	const element_t &getReadOnly(const Index3 &index);

	Index3 toOrigin(const Index3 &index);

	void clear();
	void flush();

	size_t getActivePageCount();
	size_t getPageMisses();
	void acceptXYZ(Visitor &v);

	void acceptZYX(Visitor &v);

	void acceptZYX(Visitor &v, const Index3 &l, const Index3 &u);
	void accept(Visitor &v, const Index3 &l, const Index3 &u);

};

template<typename ELEMENT>
inline PagedGrid<ELEMENT>::PagedGrid() :
	lastPage(0), pageSize(0), io(0), size(0), strategy(0), pageMisses(0) {
}

template<typename ELEMENT>
inline PagedGrid<ELEMENT>::~PagedGrid() {
	clear();
}

/// set the number of elemets per axis
template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setSize(size_t size) {
	this->size = size;
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setStrategy(PagingStrategy<element_t> *strategy) {
	this->strategy = strategy;
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setIO(PageIO<element_t> *io) {
	this->io = io;
	if (io) {
		io->setElementsPerPage(pageSize);
	}
}

/// set the number of elemets per axis per page
template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setPageSize(uint32_t pageSize) {
	assert(this->pageSize == 0 && "page size already set!");
	this->pageSize = pageSize;
	if (io) {
		io->setElementsPerPage(pageSize);
	}
}

/// set the number of pages held in memory
template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::setPageCount(size_t count) {
	if (pageSize == 0)
		throw std::runtime_error("[PagedGrid::setPageCount] page size not set!");
	if (pages.size() != 0)
		throw std::runtime_error(
				"[PagedGrid::setPageCount] page count already set!");
	if (count == 0)
		throw std::runtime_error("[PagedGrid::setPageCount] use count > 0 !");
	pages.resize(count);
	elements.resize(count * pageSize * pageSize * pageSize);
}

template<typename ELEMENT>
inline typename PagedGrid<ELEMENT>::element_t &PagedGrid<ELEMENT>::getReadWrite(
		const Index3 &index) {
	page_t *page = getPage(index);
	strategy->accessed(page);
	page->dirty = true;
	return page->get(index, pageSize);
}

template<typename ELEMENT>
inline const typename PagedGrid<ELEMENT>::element_t &PagedGrid<ELEMENT>::getReadOnly(
		const Index3 &index) {
	page_t *page = getPage(index);
	strategy->accessed(page);
	return page->get(index, pageSize);
}

template<typename ELEMENT>
inline typename PagedGrid<ELEMENT>::page_t *PagedGrid<ELEMENT>::getPage(
		const Index3 &index) {
	Index3 orig = toOrigin(index);

	if (lastPage && (lastPage->origin == orig))
		return lastPage;

	page_index_iterator_t i = pageIndex.find(orig);
	if (i != pageIndex.end()) {
		assert(i->first == orig);
		assert(i->second->origin == orig);
		return i->second;
	}

	// check if page is loaded
	page_t *page = getEmptyPage();
	if (page == 0) {
		// ask the paging strategy which page we should replace
		pageMisses++;
		page = strategy->which(pages);

		// save and clear old page
		io->savePage(page);
		strategy->cleared(page);
		pageIndex.erase(page->origin);
	}

	// load new page
	page->origin = orig;
	io->loadPage(page);
	strategy->loaded(page);
	lastPage = page;

	pageIndex[orig] = page;

	return page;
}

template<typename ELEMENT>
inline Index3 PagedGrid<ELEMENT>::toOrigin(const Index3 &index) {
	Index3 origin;
	origin.x = size_t(index.x / pageSize) * pageSize;
	origin.y = size_t(index.y / pageSize) * pageSize;
	origin.z = size_t(index.z / pageSize) * pageSize;
	return origin;
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::clear() {
	elements.clear();
	pages.clear();
}

template<typename ELEMENT>
inline typename PagedGrid<ELEMENT>::page_t *PagedGrid<ELEMENT>::getEmptyPage() {
	if (pageIndex.size() >= pages.size())
		return 0;

	// try second part first
	for (size_t i = pageIndex.size(); i < pages.size(); i++) {
		if (pages[i].elements == 0) {
			pages[i].elements = &elements.at(i * pageSize * pageSize * pageSize);
			return &pages[i];
		}
	}

	// now try first part
	for (size_t i = 0; i < pageIndex.size(); i++) {
		if (pages[i].elements == 0) {
			pages[i].elements = &elements[i * pageSize * pageSize * pageSize];
			return &pages[i];
		}
	}

	throw std::runtime_error("no empty page found.");
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::flush() {
	for (size_t i = 0; i < pages.size(); i++) {
		if (pages[i].elements) {
			io->savePage(&pages[i]);
		}
	}
}

template<typename ELEMENT>
inline size_t PagedGrid<ELEMENT>::getActivePageCount() {
	return pageIndex.size();
}

template<typename ELEMENT>
inline size_t PagedGrid<ELEMENT>::getPageMisses() {
	return pageMisses;
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::acceptXYZ(Visitor &v) {
	for (size_t iX = 0; iX < size; iX++) {
		for (size_t iY = 0; iY < size; iY++) {
			for (size_t iZ = 0; iZ < size; iZ++) {
				v.visit(*this, iX, iY, iZ, getReadWrite(iX, iY, iZ));
			}
		}
	}
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::acceptZYX(Visitor &v) {
	for (size_t iZ = 0; iZ < size; iZ++) {
		for (size_t iY = 0; iY < size; iY++) {
			for (size_t iX = 0; iX < size; iX++) {
				v.visit(*this, iX, iY, iZ, getReadWrite(iX, iY, iZ));
			}
		}
	}
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::acceptZYX(Visitor &v, const Index3 &l,
		const Index3 &u) {
	Index3 index;
	Index3 upper = u.maxByElement(Index3(size));
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

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::accept(Visitor &v, const Index3 &lower,
		const Index3 &u) {
	Index3 upper = u.minByElement(Index3(size));
	if (upper.x == 0 || upper.y == 0 || upper.z == 0) {
		return;
	}

	Index3 lowerPage = lower / pageSize;
	Index3 upperPage = (upper - Index3(1)) / pageSize;

	Index3 pageIndex;
	for (pageIndex.x = lowerPage.x; pageIndex.x <= upperPage.x; pageIndex.x++) {
		for (pageIndex.y = lowerPage.y; pageIndex.y <= upperPage.y; pageIndex.y++) {
			for (pageIndex.z = lowerPage.z; pageIndex.z <= upperPage.z; pageIndex.z++) {
				page_t *page = getPage(pageIndex * pageSize);
				assert(page != 0);
				strategy->accessed(page);
				page->dirty = true;
				pageAccept(page, v, lower, upper);
			}
		}
	}
}

template<typename ELEMENT>
inline void PagedGrid<ELEMENT>::pageAccept(Page<ELEMENT> *page, Visitor &v,
		const Index3 &l, const Index3 &u) {
	Index3 lower = l.maxByElement(page->origin);
	Index3 upper = u.minByElement(page->origin + Index3(pageSize));
	uint32_t pageSize2 = pageSize * pageSize;
	Index3 offset = lower - page->origin;

	for (uint32_t z = lower.z; z < upper.z; z++) {
		offset.z = (z - page->origin.z) * pageSize2;
		for (uint32_t y = lower.y; y < upper.y; y++) {
			offset.y = (y - page->origin.y) * pageSize;
			for (uint32_t x = lower.x; x < upper.x; x++) {
				offset.x = x - page->origin.x;
				v.visit(*this, x, y, z, page->elements[offset.x + offset.y
						+ offset.z]);
			}
		}
	}

}
#endif /* PAGED_GRID_HPP_ */
