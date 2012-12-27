// $Id$

#include "EmptyDiskPatch.hh"
#include "SectorAccessibleDisk.hh"
#include <cassert>

namespace openmsx {

EmptyDiskPatch::EmptyDiskPatch(SectorAccessibleDisk& disk_)
	: disk(disk_)
{
}

void EmptyDiskPatch::copyBlock(size_t src, byte* dst, size_t num) const
{
	(void)num;
	assert(num == SectorAccessibleDisk::SECTOR_SIZE);
	disk.readSectorImpl(src / SectorAccessibleDisk::SECTOR_SIZE, dst);
}

size_t EmptyDiskPatch::getSize() const
{
	return disk.getNbSectors() * SectorAccessibleDisk::SECTOR_SIZE;
}

std::vector<Filename> EmptyDiskPatch::getFilenames() const
{
	// return {};
	return std::vector<Filename>();
}

} // namespace openmsx

