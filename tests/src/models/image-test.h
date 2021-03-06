#ifndef IMAGE_TEST_H
#define IMAGE_TEST_H

#include "models/image.h"
#include "models/site.h"
#include "models/source.h"
#include "test-suite.h"


class ImageTest : public TestSuite
{
	Q_OBJECT

	private slots:
		void init();
		void cleanup();

		void testConstructor();
		void testCopy();
		void testHasTag();
		void testHasAnyTag();
		void testHasAllTags();
		void testMd5FromData();
		//void testMd5FromFile();
		void testStylishedTags();
		void testValue();
		void testLoadDetails();
		void testLoadDetailsAbort();
		void testLoadDetailsImageUrl();
		void testSave();
#ifdef Q_OS_WIN
		void testSaveError();
#endif
		void testSaveAlreadyExists();
		void testSaveDuplicate();
		void testSaveLog();
		void testSetUrl();

	private:
		Profile *m_profile;
		QSettings *m_settings;
		Source *m_source;
		Site *m_site;
		Image *m_img;
		QMap<QString, QString> m_details;
};

#endif // IMAGE_TEST_H
