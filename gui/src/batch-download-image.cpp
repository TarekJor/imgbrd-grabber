#include "batch-download-image.h"


const DownloadQuery *BatchDownloadImage::query() const
{
	if (queryGroup != nullptr)
		return queryGroup;
	return queryImage;
}

int BatchDownloadImage::siteId(const QList<DownloadQueryGroup> &groups) const
{
	if (queryGroup != nullptr)
		return groups.indexOf(*queryGroup) + 1;
	return -1;
}


bool operator==(const BatchDownloadImage &lhs, const BatchDownloadImage &rhs)
{
	return lhs.image == rhs.image;
}
