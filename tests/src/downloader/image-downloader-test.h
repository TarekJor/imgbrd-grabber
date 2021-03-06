#ifndef IMAGE_DOWNLOADER_TEST_H
#define IMAGE_DOWNLOADER_TEST_H

#include "downloader/image-downloader.h"
#include "models/source.h"
#include "test-suite.h"


class ImageDownloaderTest : public TestSuite
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void cleanup();

		void testSuccessBasic();
		void testSuccessLoadTags();
		void testOpenError();
		void testNotFound();
		void testNetworkError();
		void testOriginalMd5();
		void testGeneratedMd5();
		void testRotateExtension();

	protected:
		Image *createImage(bool noMd5 = false);
		void assertDownload(QSharedPointer<Image> img, ImageDownloader *downloader, const QMap<QString, Image::SaveResult> &expected, bool shouldExist, bool onlyCheckValues = false);

	private:
		Profile *m_profile;
		Source *m_source;
		Site *m_site;
};

#endif // IMAGE_DOWNLOADER_TEST_H
