#include "language-loader.h"
#include <QDir>
#include <QSettings>


LanguageLoader::LanguageLoader(QString path)
	: m_path(std::move(path))
{}

QMap<QString, QString> LanguageLoader::getAllLanguages() const
{
	QSettings fullLanguages(m_path + "languages.ini", QSettings::IniFormat);
	fullLanguages.setIniCodec("UTF-8");

	QStringList languageFiles = QDir(m_path).entryList(QStringList() << QStringLiteral("*.qm"), QDir::Files);
	QMap<QString, QString> languages;
	for (const QString &languageFile : languageFiles)
	{
		const QString lang = languageFile.left(languageFile.length() - 3);
		const QString fullLang = fullLanguages.value(lang, lang).toString();
		languages[lang] = fullLang;
	}

	if (!languages.contains("English"))
	{
		languages[""] = "English";
	}

	return languages;
}
