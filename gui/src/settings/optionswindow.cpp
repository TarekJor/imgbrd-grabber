#include "settings/optionswindow.h"
#include <QColorDialog>
#include <QFileDialog>
#include <QFontDialog>
#include <QNetworkProxy>
#include <QSignalMapper>
#include <QSqlDatabase>
#include <algorithm>
#include <ui_optionswindow.h>
#include "functions.h"
#include "helpers.h"
#include "language-loader.h"
#include "models/profile.h"
#include "models/site.h"
#include "reverse-search/reverse-search-loader.h"
#include "settings/conditionwindow.h"
#include "settings/customwindow.h"
#include "settings/filenamewindow.h"
#include "settings/log-window.h"
#include "settings/web-service-window.h"
#include "theme-loader.h"


optionsWindow::optionsWindow(Profile *profile, QWidget *parent)
	: QDialog(parent), ui(new Ui::optionsWindow), m_profile(profile)
{
	setAttribute(Qt::WA_DeleteOnClose);
	ui->setupUi(this);

	ui->splitter->setSizes(QList<int>() << 160 << ui->stackedWidget->sizeHint().width());
	ui->splitter->setStretchFactor(0, 0);
	ui->splitter->setStretchFactor(1, 1);

	LanguageLoader languageLoader(savePath("languages/", true));
	QMap<QString, QString> languages = languageLoader.getAllLanguages();
	for (auto it = languages.begin(); it != languages.end(); ++it)
	{ ui->comboLanguages->addItem(it.value(), it.key()); }

	QSettings *settings = profile->getSettings();
	ui->comboLanguages->setCurrentText(languages[settings->value("language", "English").toString()]);
	ui->lineWhitelist->setText(settings->value("whitelistedtags").toString());
	ui->lineAdd->setText(settings->value("add").toString());
	QStringList wl = QStringList() << "never" << "image" << "page";
	ui->comboWhitelist->setCurrentIndex(wl.indexOf(settings->value("whitelist_download", "image").toString()));
	ui->lineIgnored->setText(settings->value("ignoredtags").toString());
	QStringList starts = QStringList() << "none" << "loadfirst" << "restore";
	ui->comboStart->setCurrentIndex(starts.indexOf(settings->value("start", "none").toString()));
	ui->spinHideFavorites->setValue(settings->value("hidefavorites", 20).toInt());
	ui->checkAutodownload->setChecked(settings->value("autodownload", false).toBool());
	ui->checkHideBlacklisted->setChecked(settings->value("hideblacklisted", false).toBool());
	ui->checkShowTagWarning->setChecked(settings->value("showtagwarning", true).toBool());
	ui->checkShowWarnings->setChecked(settings->value("showwarnings", true).toBool());
	ui->checkGetUnloadedPages->setChecked(settings->value("getunloadedpages", false).toBool());
	ui->checkInvertToggle->setChecked(settings->value("invertToggle", false).toBool());
	ui->checkConfirmClose->setChecked(settings->value("confirm_close", true).toBool());
	QList<int> checkForUpdates = QList<int>() << 0 << 24 * 60 * 60 << 7 * 24 * 60 * 60 << 30 * 24 * 60 * 60 << -1;
	ui->comboCheckForUpdates->setCurrentIndex(checkForUpdates.indexOf(settings->value("check_for_updates", 24 * 60 * 60).toInt()));

	ui->spinImagesPerPage->setValue(settings->value("limit", 20).toInt());
	ui->spinColumns->setValue(settings->value("columns", 1).toInt());
	QStringList sources = QStringList() << "xml" << "json" << "regex" << "rss";
	ui->comboSource1->setCurrentIndex(sources.indexOf(settings->value("source_1", "xml").toString()));
	ui->comboSource2->setCurrentIndex(sources.indexOf(settings->value("source_2", "json").toString()));
	ui->comboSource3->setCurrentIndex(sources.indexOf(settings->value("source_3", "regex").toString()));
	ui->comboSource4->setCurrentIndex(sources.indexOf(settings->value("source_4", "rss").toString()));
	ui->spinAutoTagAdd->setValue(settings->value("tagsautoadd", 10).toInt());

	QMap<QString, QPair<QString, QString>> filenames = getFilenames(settings);
	m_filenamesConditions = QList<QLineEdit*>();
	m_filenamesFilenames = QList<QLineEdit*>();
	for (auto it = filenames.begin(); it != filenames.end(); ++it)
	{
		auto leCondition = new QLineEdit(it.key());
		auto leFilename = new QLineEdit(it.value().first);
		auto leFolder = new QLineEdit(it.value().second);

		m_filenamesConditions.append(leCondition);
		m_filenamesFilenames.append(leFilename);
		m_filenamesFolders.append(leFolder);

		auto *layout = new QHBoxLayout;
		layout->addWidget(leCondition);
		layout->addWidget(leFilename);
		layout->addWidget(leFolder);
		ui->layoutConditionals->addLayout(layout);
	}
	QStringList types = QStringList() << "text" << "icon" << "both" << "hide";
	ui->comboSources->setCurrentIndex(types.indexOf(settings->value("Sources/Types", "icon").toString()));
	int i = settings->value("Sources/Letters", 3).toInt();
	ui->comboSourcesLetters->setCurrentIndex((i < 0 ? 1 : 0) + (i < -1 ? 1 : 0));
	ui->spinSourcesLetters->setValue(i < 0 ? 3 : i);
	ui->checkPreloadAllTabs->setChecked(settings->value("preloadAllTabs", false).toBool());

	QStringList ftypes = QStringList() << "ind" << "in" << "id" << "nd" << "i" << "n" << "d";
	ui->comboFavoritesDisplay->setCurrentIndex(ftypes.indexOf(settings->value("favorites_display", "ind").toString()));

	// Log
	settings->beginGroup("Log");
		ui->checkShowLog->setChecked(settings->value("show", true).toBool());
	settings->endGroup();

	// Blacklist
	ui->textBlacklist->setPlainText(profile->getBlacklist().toString());
	ui->checkDownloadBlacklisted->setChecked(settings->value("downloadblacklist", false).toBool());

	// Monitoring
	settings->beginGroup("Monitoring");
		ui->spinMonitoringStartupDelay->setValue(settings->value("startupDelay", 0).toInt());
		ui->checkMonitoringEnableTray->setChecked(settings->value("enableTray", false).toBool());
		ui->checkMonitoringMinimizeToTray->setChecked(settings->value("minimizeToTray", false).toBool());
		ui->checkMonitoringCloseToTray->setChecked(settings->value("closeToTray", false).toBool());
	settings->endGroup();

	ui->checkResizeInsteadOfCropping->setChecked(settings->value("resizeInsteadOfCropping", true).toBool());
	ui->spinThumbnailUpscale->setValue(qRound(settings->value("thumbnailUpscale", 1.0).toDouble() * 100));
	ui->checkAutocompletion->setChecked(settings->value("autocompletion", true).toBool());
	ui->checkUseregexfortags->setChecked(settings->value("useregexfortags", true).toBool());
	QStringList infiniteScroll = QStringList() << "disabled" << "button" << "scroll";
	ui->comboInfiniteScroll->setCurrentIndex(infiniteScroll.indexOf(settings->value("infiniteScroll", "disabled").toString()));
	ui->checkInfiniteScrollRememberPage->setChecked(settings->value("infiniteScrollRememberPage", false).toBool());

	// External log files
	showLogFiles(settings);

	// Web services
	ReverseSearchLoader loader(settings);
	m_webServices = loader.getAllReverseSearchEngines();
	showWebServices();

	ui->comboBatchEnd->setCurrentIndex(settings->value("Batch/end", 0).toInt());
	settings->beginGroup("Save");
		ui->spinAutomaticRetries->setValue(settings->value("automaticretries", 0).toInt());
		ui->checkDownloadOriginals->setChecked(settings->value("downloadoriginals", true).toBool());
		ui->checkSampleFallback->setChecked(settings->value("samplefallback", true).toBool());
		ui->checkReplaceBlanks->setChecked(settings->value("replaceblanks", false).toBool());
		ui->checkKeepDate->setChecked(settings->value("keepDate", true).toBool());
		ui->checkSaveHeaderDetection->setChecked(settings->value("headerDetection", true).toBool());
		ui->lineFolder->setText(settings->value("path_real").toString());
		ui->lineFolderFavorites->setText(settings->value("path_favorites").toString());
		QStringList opts = QStringList() << "save" << "copy" << "move" << "ignore";
		ui->comboMd5Duplicates->setCurrentIndex(opts.indexOf(settings->value("md5Duplicates", "save").toString()));
		ui->checkKeepDeletedMd5->setChecked(settings->value("keepDeletedMd5", false).toBool());

		ui->lineFilename->setText(settings->value("filename_real").toString());
		ui->lineFavorites->setText(settings->value("filename_favorites").toString());
		ui->lineSeparator->setText(settings->value("separator", " ").toString());
		ui->checkNoJpeg->setChecked(settings->value("noJpeg", true).toBool());

		ui->lineArtistsIfNone->setText(settings->value("artist_empty", "anonymous").toString());
		ui->spinArtistsMoreThanN->setValue(settings->value("artist_multiple_limit", 1).toInt());
		ui->spinArtistsKeepN->setValue(settings->value("artist_multiple_keepN", 1).toInt());
		ui->spinArtistsKeepNThenAdd->setValue(settings->value("artist_multiple_keepNThenAdd_keep", 1).toInt());
		ui->lineArtistsKeepNThenAdd->setText(settings->value("artist_multiple_keepNThenAdd_add", " (+ %count%)").toString());
		ui->lineArtistsSeparator->setText(settings->value("artist_sep", "+").toString());
		ui->lineArtistsReplaceAll->setText(settings->value("artist_value", "multiple artists").toString());
		const QString artistMultiple = settings->value("artist_multiple", "replaceAll").toString();
		if		(artistMultiple == "keepAll")		{ ui->radioArtistsKeepAll->setChecked(true);		}
		else if	(artistMultiple == "keepN")			{ ui->radioArtistsKeepN->setChecked(true);			}
		else if	(artistMultiple == "keepNThenAdd")	{ ui->radioArtistsKeepNThenAdd->setChecked(true);	}
		else if	(artistMultiple == "replaceAll")	{ ui->radioArtistsReplaceAll->setChecked(true);		}
		else if	(artistMultiple == "multiple")		{ ui->radioArtistsMultiple->setChecked(true);		}

		ui->lineCopyrightsIfNone->setText(settings->value("copyright_empty", "misc").toString());
		ui->checkCopyrightsUseShorter->setChecked(settings->value("copyright_useshorter", true).toBool());
		ui->spinCopyrightsMoreThanN->setValue(settings->value("copyright_multiple_limit", 1).toInt());
		ui->spinCopyrightsKeepN->setValue(settings->value("copyright_multiple_keepN", 1).toInt());
		ui->spinCopyrightsKeepNThenAdd->setValue(settings->value("copyright_multiple_keepNThenAdd_keep", 1).toInt());
		ui->lineCopyrightsKeepNThenAdd->setText(settings->value("copyright_multiple_keepNThenAdd_add", " (+ %count%)").toString());
		ui->lineCopyrightsSeparator->setText(settings->value("copyright_sep", "+").toString());
		ui->lineCopyrightsReplaceAll->setText(settings->value("copyright_value", "crossover").toString());
		const QString copyrightMultiple = settings->value("copyright_multiple", "replaceAll").toString();
		if		(copyrightMultiple == "keepAll")		{ ui->radioCopyrightsKeepAll->setChecked(true);			}
		else if	(copyrightMultiple == "keepN")			{ ui->radioCopyrightsKeepN->setChecked(true);			}
		else if	(copyrightMultiple == "keepNThenAdd")	{ ui->radioCopyrightsKeepNThenAdd->setChecked(true);	}
		else if	(copyrightMultiple == "replaceAll")		{ ui->radioCopyrightsReplaceAll->setChecked(true);		}
		else if	(copyrightMultiple == "multiple")		{ ui->radioCopyrightsMultiple->setChecked(true);		}

		ui->lineCharactersIfNone->setText(settings->value("character_empty", "unknown").toString());
		ui->spinCharactersMoreThanN->setValue(settings->value("character_multiple_limit", 1).toInt());
		ui->spinCharactersKeepN->setValue(settings->value("character_multiple_keepN", 1).toInt());
		ui->spinCharactersKeepNThenAdd->setValue(settings->value("character_multiple_keepNThenAdd_keep", 1).toInt());
		ui->lineCharactersKeepNThenAdd->setText(settings->value("character_multiple_keepNThenAdd_add", " (+ %count%)").toString());
		ui->lineCharactersSeparator->setText(settings->value("character_sep", "+").toString());
		ui->lineCharactersReplaceAll->setText(settings->value("character_value", "group").toString());
		const QString characterMultiple = settings->value("character_multiple", "replaceAll").toString();
		if		(characterMultiple == "keepAll")		{ ui->radioCharactersKeepAll->setChecked(true);			}
		else if	(characterMultiple == "keepN")			{ ui->radioCharactersKeepN->setChecked(true);			}
		else if	(characterMultiple == "keepNThenAdd")	{ ui->radioCharactersKeepNThenAdd->setChecked(true);	}
		else if	(characterMultiple == "replaceAll")		{ ui->radioCharactersReplaceAll->setChecked(true);		}
		else if	(characterMultiple == "multiple")		{ ui->radioCharactersMultiple->setChecked(true);		}

		ui->lineSpeciesIfNone->setText(settings->value("species_empty", "unknown").toString());
		ui->spinSpeciesMoreThanN->setValue(settings->value("species_multiple_limit", 1).toInt());
		ui->spinSpeciesKeepN->setValue(settings->value("species_multiple_keepN", 1).toInt());
		ui->spinSpeciesKeepNThenAdd->setValue(settings->value("species_multiple_keepNThenAdd_keep", 1).toInt());
		ui->lineSpeciesKeepNThenAdd->setText(settings->value("species_multiple_keepNThenAdd_add", " (+ %count%)").toString());
		ui->lineSpeciesSeparator->setText(settings->value("species_sep", "+").toString());
		ui->lineSpeciesReplaceAll->setText(settings->value("species_value", "multiple").toString());
		const QString speciesMultiple = settings->value("species_multiple", "keepAll").toString();
		if		(speciesMultiple == "keepAll")		{ ui->radioSpeciesKeepAll->setChecked(true);		}
		else if	(speciesMultiple == "keepN")		{ ui->radioSpeciesKeepN->setChecked(true);			}
		else if	(speciesMultiple == "keepNThenAdd")	{ ui->radioSpeciesKeepNThenAdd->setChecked(true);	}
		else if	(speciesMultiple == "replaceAll")	{ ui->radioSpeciesReplaceAll->setChecked(true);		}
		else if	(speciesMultiple == "multiple")		{ ui->radioSpeciesMultiple->setChecked(true);		}

		ui->lineMetasIfNone->setText(settings->value("meta_empty", "none").toString());
		ui->spinMetasMoreThanN->setValue(settings->value("meta_multiple_limit", 1).toInt());
		ui->spinMetasKeepN->setValue(settings->value("meta_multiple_keepN", 1).toInt());
		ui->spinMetasKeepNThenAdd->setValue(settings->value("meta_multiple_keepNThenAdd_keep", 1).toInt());
		ui->lineMetasKeepNThenAdd->setText(settings->value("meta_multiple_keepNThenAdd_add", " (+ %count%)").toString());
		ui->lineMetasSeparator->setText(settings->value("meta_sep", "+").toString());
		ui->lineMetasReplaceAll->setText(settings->value("meta_value", "multiple").toString());
		const QString metaMultiple = settings->value("meta_multiple", "keepAll").toString();
		if		(metaMultiple == "keepAll")			{ ui->radioMetasKeepAll->setChecked(true);		}
		else if	(metaMultiple == "keepN")			{ ui->radioMetasKeepN->setChecked(true);		}
		else if	(metaMultiple == "keepNThenAdd")	{ ui->radioMetasKeepNThenAdd->setChecked(true);	}
		else if	(metaMultiple == "replaceAll")		{ ui->radioMetasReplaceAll->setChecked(true);	}
		else if	(metaMultiple == "multiple")		{ ui->radioMetasMultiple->setChecked(true);		}

		ui->spinLimit->setValue(settings->value("limit", 0).toInt());
		ui->spinSimultaneous->setValue(settings->value("simultaneous", 1).toInt());
	settings->endGroup();

	// Custom tokens
	QMap<QString, QStringList> customs = getCustoms(settings);
	m_customNames = QList<QLineEdit*>();
	m_customTags = QList<QLineEdit*>();
	i = 0;
	for (auto it = customs.begin(); it != customs.end(); ++it)
	{
		auto *leName = new QLineEdit(it.key());
		auto *leTags = new QLineEdit(it.value().join(" "));
		m_customNames.append(leName);
		m_customTags.append(leTags);
		ui->layoutCustom->insertRow(i++, leName, leTags);
	}

	// Themes
	ThemeLoader themeLoader(savePath("themes/", true));
	QStringList themes = themeLoader.getAllThemes();
	for (const QString &theme : themes)
	{ ui->comboTheme->addItem(theme, theme); }
	ui->comboTheme->setCurrentText(settings->value("theme", "Default").toString());

	ui->checkSingleDetailsWindow->setChecked(settings->value("Zoom/singleWindow", false).toBool());
	QStringList positions = QStringList() << "top" << "left" << "auto";
	ui->comboTagsPosition->setCurrentIndex(positions.indexOf(settings->value("tagsposition", "top").toString()));
	ui->spinPreload->setValue(settings->value("preload", 0).toInt());
	ui->spinSlideshow->setValue(settings->value("slideshow", 0).toInt());
	ui->checkResultsScrollArea->setChecked(settings->value("resultsScrollArea", true).toBool());
	ui->checkResultsFixedWidthLayout->setChecked(settings->value("resultsFixedWidthLayout", false).toBool());
	ui->checkImageCloseMiddleClick->setChecked(settings->value("imageCloseMiddleClick", true).toBool());
	ui->checkImageNavigateScroll->setChecked(settings->value("imageNavigateScroll", true).toBool());
	ui->checkZoomShowTagCount->setChecked(settings->value("Zoom/showTagCount", false).toBool());
	//ui->checkZoomViewSamples->setChecked(settings->value("Zoom/viewSamples", false).toBool());
	QStringList imageTagOrder = QStringList() << "type" << "name" << "count";
	ui->comboImageTagOrder->setCurrentIndex(imageTagOrder.indexOf(settings->value("Zoom/tagOrder", "type").toString()));
	QStringList positionsV = QStringList() << "top" << "center" << "bottom";
	QStringList positionsH = QStringList() << "left" << "center" << "right";
	ui->comboImagePositionImageV->setCurrentIndex(positionsV.indexOf(settings->value("imagePositionImageV", "center").toString()));
	ui->comboImagePositionImageH->setCurrentIndex(positionsH.indexOf(settings->value("imagePositionImageH", "left").toString()));
	ui->comboImagePositionAnimationV->setCurrentIndex(positionsV.indexOf(settings->value("imagePositionAnimationV", "center").toString()));
	ui->comboImagePositionAnimationH->setCurrentIndex(positionsH.indexOf(settings->value("imagePositionAnimationH", "left").toString()));
	ui->comboImagePositionVideoV->setCurrentIndex(positionsV.indexOf(settings->value("imagePositionVideoV", "center").toString()));
	ui->comboImagePositionVideoH->setCurrentIndex(positionsH.indexOf(settings->value("imagePositionVideoH", "left").toString()));
	ui->lineImageBackgroundColor->setText(settings->value("imageBackgroundColor", QString()).toString());

	settings->beginGroup("Coloring");
		settings->beginGroup("Colors");
			ui->lineColoringArtists->setText(settings->value("artists", "#aa0000").toString());
			ui->lineColoringCircles->setText(settings->value("circles", "#55bbff").toString());
			ui->lineColoringCopyrights->setText(settings->value("copyrights", "#aa00aa").toString());
			ui->lineColoringCharacters->setText(settings->value("characters", "#00aa00").toString());
			ui->lineColoringSpecies->setText(settings->value("species", "#ee6600").toString());
			ui->lineColoringMetas->setText(settings->value("metas", "#ee6600").toString());
			ui->lineColoringModels->setText(settings->value("models", "#0000ee").toString());
			ui->lineColoringGenerals->setText(settings->value("generals", "#000000").toString());
			ui->lineColoringFavorites->setText(settings->value("favorites", "#ffc0cb").toString());
			ui->lineColoringKeptForLater->setText(settings->value("keptForLater", "#000000").toString());
			ui->lineColoringBlacklisteds->setText(settings->value("blacklisteds", "#000000").toString());
			ui->lineColoringIgnoreds->setText(settings->value("ignoreds", "#999999").toString());
		settings->endGroup();
		settings->beginGroup("Fonts");
			ui->lineColoringArtists->setFont(qFontFromString(settings->value("artists").toString()));
			ui->lineColoringCircles->setFont(qFontFromString(settings->value("circles").toString()));
			ui->lineColoringCopyrights->setFont(qFontFromString(settings->value("copyrights").toString()));
			ui->lineColoringCharacters->setFont(qFontFromString(settings->value("characters").toString()));
			ui->lineColoringSpecies->setFont(qFontFromString(settings->value("species").toString()));
			ui->lineColoringMetas->setFont(qFontFromString(settings->value("metas").toString()));
			ui->lineColoringModels->setFont(qFontFromString(settings->value("models").toString()));
			ui->lineColoringGenerals->setFont(qFontFromString(settings->value("generals").toString()));
			ui->lineColoringFavorites->setFont(qFontFromString(settings->value("favorites").toString()));
			ui->lineColoringKeptForLater->setFont(qFontFromString(settings->value("keptForLater").toString()));
			ui->lineColoringBlacklisteds->setFont(qFontFromString(settings->value("blacklisteds").toString()));
			ui->lineColoringIgnoreds->setFont(qFontFromString(settings->value("ignoreds").toString()));
		settings->endGroup();
	settings->endGroup();

	settings->beginGroup("Margins");
		ui->spinMainMargins->setValue(settings->value("main", 10).toInt());
		ui->spinHorizontalMargins->setValue(settings->value("horizontal", 6).toInt());
		ui->spinVerticalMargins->setValue(settings->value("vertical", 6).toInt());
	settings->endGroup();
	ui->spinServerBorders->setValue(settings->value("serverBorder", 0).toInt());
	ui->lineBorderColor->setText(settings->value("serverBorderColor", "#000000").toString());
	ui->spinBorders->setValue(settings->value("borders", 3).toInt());

	settings->beginGroup("Proxy");
		ui->checkProxyUse->setChecked(settings->value("use", false).toBool());
		ui->checkProxyUseSystem->setChecked(settings->value("useSystem", false).toBool());
		QStringList ptypes = QStringList() << "http" << "socks5";
		ui->comboProxyType->setCurrentIndex(ptypes.indexOf(settings->value("type", "http").toString()));
		ui->widgetProxy->setEnabled(settings->value("use", false).toBool());
		ui->lineProxyHostName->setText(settings->value("hostName").toString());
		ui->spinProxyPort->setValue(settings->value("port").toInt());
		ui->lineProxyUser->setText(settings->value("user").toString());
		ui->lineProxyPassword->setText(settings->value("password").toString());
	settings->endGroup();

	settings->beginGroup("Exec");
		ui->lineCommandsTagBefore->setText(settings->value("tag_before").toString());
		ui->lineCommandsImage->setText(settings->value("image").toString());
		ui->lineCommandsTagAfter->setText(settings->value("tag_after", settings->value("tag").toString()).toString());
		settings->beginGroup("SQL");
			ui->comboCommandsSqlDriver->addItems(QSqlDatabase::drivers());
			ui->comboCommandsSqlDriver->setCurrentIndex(QSqlDatabase::drivers().indexOf(settings->value("driver", "QMYSQL").toString()));
			ui->lineCommandsSqlHost->setText(settings->value("host").toString());
			ui->lineCommandsSqlUser->setText(settings->value("user").toString());
			ui->lineCommandsSqlPassword->setText(settings->value("password").toString());
			ui->lineCommandsSqlDatabase->setText(settings->value("database").toString());
			ui->lineCommandsSqlBefore->setText(settings->value("before").toString());
			ui->lineCommandsSqlTagBefore->setText(settings->value("tag_before").toString());
			ui->lineCommandsSqlImage->setText(settings->value("image").toString());
			ui->lineCommandsSqlTagAfter->setText(settings->value("tag_after", settings->value("tag").toString()).toString());
			ui->lineCommandsSqlAfter->setText(settings->value("after").toString());
		settings->endGroup();
	settings->endGroup();

	connect(this, &QDialog::accepted, this, &optionsWindow::save);
}

optionsWindow::~optionsWindow()
{
	delete ui;
}

void optionsWindow::on_comboSourcesLetters_currentIndexChanged(int i)
{ ui->spinSourcesLetters->setDisabled(i > 0); }

void optionsWindow::on_buttonFolder_clicked()
{
	QString folder = QFileDialog::getExistingDirectory(this, tr("Choose a save folder"), ui->lineFolder->text());
	if (!folder.isEmpty())
	{ ui->lineFolder->setText(folder); }
}
void optionsWindow::on_buttonFolderFavorites_clicked()
{
	QString folder = QFileDialog::getExistingDirectory(this, tr("Choose a save folder for favorites"), ui->lineFolderFavorites->text());
	if (!folder.isEmpty())
	{ ui->lineFolderFavorites->setText(folder); }
}

void optionsWindow::on_buttonFilenamePlus_clicked()
{
	FilenameWindow *fw = new FilenameWindow(m_profile, ui->lineFilename->text(), this);
	connect(fw, &FilenameWindow::validated, ui->lineFilename, &QLineEdit::setText);
	fw->show();
}
void optionsWindow::on_buttonFavoritesPlus_clicked()
{
	FilenameWindow *fw = new FilenameWindow(m_profile, ui->lineFavorites->text(), this);
	connect(fw, &FilenameWindow::validated, ui->lineFavorites, &QLineEdit::setText);
	fw->show();
}

void optionsWindow::on_buttonCustom_clicked()
{
	auto *cw = new CustomWindow(this);
	connect(cw, &CustomWindow::validated, this, &optionsWindow::addCustom);
	cw->show();
}
void optionsWindow::addCustom(const QString &name, const QString &tags)
{
	auto *leName = new QLineEdit(name);
	auto *leTags = new QLineEdit(tags);
	ui->layoutCustom->insertRow(m_customNames.size(), leName, leTags);
	m_customNames.append(leName);
	m_customTags.append(leTags);
}
void optionsWindow::on_buttonFilenames_clicked()
{
	auto *cw = new conditionWindow();
	connect(cw, &conditionWindow::validated, this, &optionsWindow::addFilename);
	cw->show();
}
void optionsWindow::addFilename(const QString &condition, const QString &filename, const QString &folder)
{
	auto *leCondition = new QLineEdit(condition);
	auto *leFilename = new QLineEdit(filename);
	auto *leFolder = new QLineEdit(folder);

	m_filenamesConditions.append(leCondition);
	m_filenamesFilenames.append(leFilename);
	m_filenamesFolders.append(leFolder);

	auto *layout = new QHBoxLayout(this);
	layout->addWidget(leCondition);
	layout->addWidget(leFilename);
	layout->addWidget(leFolder);
	ui->layoutConditionals->addLayout(layout);
}


void optionsWindow::showLogFiles(QSettings *settings)
{
	clearLayout(ui->layoutLogFiles);

	auto logFiles = getExternalLogFiles(settings);
	auto *mapperEditLogFile = new QSignalMapper(this);
	auto *mapperRemoveLogFile = new QSignalMapper(this);
	connect(mapperEditLogFile, SIGNAL(mapped(int)), this, SLOT(editLogFile(int)));
	connect(mapperRemoveLogFile, SIGNAL(mapped(int)), this, SLOT(removeLogFile(int)));
	for (auto it = logFiles.begin(); it != logFiles.end(); ++it)
	{
		const int i = it.key();
		auto logFile = it.value();

		auto *label = new QLabel(logFile["name"].toString());
		label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		ui->layoutLogFiles->addWidget(label, i, 0);

		auto *buttonEdit = new QPushButton(tr("Edit"));
		mapperEditLogFile->setMapping(buttonEdit, i);
		connect(buttonEdit, SIGNAL(clicked(bool)), mapperEditLogFile, SLOT(map()));
		ui->layoutLogFiles->addWidget(buttonEdit, i, 1);

		auto *buttonDelete = new QPushButton(tr("Remove"));
		mapperRemoveLogFile->setMapping(buttonDelete, i);
		connect(buttonDelete, SIGNAL(clicked(bool)), mapperRemoveLogFile, SLOT(map()));
		ui->layoutLogFiles->addWidget(buttonDelete, i, 2);
	}
}

void optionsWindow::addLogFile()
{
	auto *logWindow = new LogWindow(-1, m_profile, this);
	connect(logWindow, &LogWindow::validated, this, &optionsWindow::setLogFile);
	logWindow->show();
}

void optionsWindow::editLogFile(int index)
{
	auto *logWindow = new LogWindow(index, m_profile, this);
	connect(logWindow, &LogWindow::validated, this, &optionsWindow::setLogFile);
	logWindow->show();
}

void optionsWindow::removeLogFile(int index)
{
	QSettings *settings = m_profile->getSettings();
	settings->beginGroup("LogFiles");
	settings->beginGroup(QString::number(index));
	for (const QString &key : settings->childKeys())
	{ settings->remove(key); }
	settings->endGroup();
	settings->endGroup();

	showLogFiles(settings);
}

void optionsWindow::setLogFile(int index, QMap<QString, QVariant> logFile)
{
	QSettings *settings = m_profile->getSettings();
	settings->beginGroup("LogFiles");

	if (index < 0)
	{
		auto childGroups = settings->childGroups();
		if (childGroups.isEmpty())
		{ index = 0; }
		else
		{ index = settings->childGroups().last().toInt() + 1; }
	}

	settings->beginGroup(QString::number(index));

	for (auto it = logFile.begin(); it != logFile.end(); ++it)
	{ settings->setValue(it.key(), it.value()); }

	settings->endGroup();
	settings->endGroup();

	showLogFiles(settings);
}


void optionsWindow::showWebServices()
{
	clearLayout(ui->layoutWebServices);

	auto *mapperEditWebService = new QSignalMapper(this);
	auto *mapperRemoveWebService = new QSignalMapper(this);
	auto *mapperMoveUpWebService = new QSignalMapper(this);
	auto *mapperMoveDownWebService = new QSignalMapper(this);
	connect(mapperEditWebService, SIGNAL(mapped(int)), this, SLOT(editWebService(int)));
	connect(mapperRemoveWebService, SIGNAL(mapped(int)), this, SLOT(removeWebService(int)));
	connect(mapperMoveUpWebService, SIGNAL(mapped(int)), this, SLOT(moveUpWebService(int)));
	connect(mapperMoveDownWebService, SIGNAL(mapped(int)), this, SLOT(moveDownWebService(int)));

	m_webServicesIds.clear();
	for (int j = 0; j < m_webServices.count(); ++j)
	{
		auto webService = m_webServices[j];
		int id = webService.id();
		m_webServicesIds.insert(id, j);

		QIcon icon = webService.icon();
		QLabel *labelIcon = new QLabel();
		labelIcon->setPixmap(icon.pixmap(QSize(16, 16)));
		labelIcon->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
		ui->layoutWebServices->addWidget(labelIcon, j, 0);

		QLabel *label = new QLabel(webService.name());
		label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		ui->layoutWebServices->addWidget(label, j, 1);

		if (j > 0)
		{
			QPushButton *buttonMoveUp = new QPushButton(QIcon(":/images/icons/arrow-up.png"), QString());
			mapperMoveUpWebService->setMapping(buttonMoveUp, id);
			connect(buttonMoveUp, SIGNAL(clicked(bool)), mapperMoveUpWebService, SLOT(map()));
			ui->layoutWebServices->addWidget(buttonMoveUp, j, 2);
		}

		if (j < m_webServices.count() - 1)
		{
			QPushButton *buttonMoveDown = new QPushButton(QIcon(":/images/icons/arrow-down.png"), QString());
			mapperMoveDownWebService->setMapping(buttonMoveDown, id);
			connect(buttonMoveDown, SIGNAL(clicked(bool)), mapperMoveDownWebService, SLOT(map()));
			ui->layoutWebServices->addWidget(buttonMoveDown, j, 3);
		}

		QPushButton *buttonEdit = new QPushButton(tr("Edit"));
		mapperEditWebService->setMapping(buttonEdit, id);
		connect(buttonEdit, SIGNAL(clicked(bool)), mapperEditWebService, SLOT(map()));
		ui->layoutWebServices->addWidget(buttonEdit, j, 4);

		QPushButton *buttonDelete = new QPushButton(tr("Remove"));
		mapperRemoveWebService->setMapping(buttonDelete, id);
		connect(buttonDelete, SIGNAL(clicked(bool)), mapperRemoveWebService, SLOT(map()));
		ui->layoutWebServices->addWidget(buttonDelete, j, 5);
	}
}

void optionsWindow::addWebService()
{
	auto *wsWindow = new WebServiceWindow(nullptr, this);
	connect(wsWindow, &WebServiceWindow::validated, this, &optionsWindow::setWebService);
	wsWindow->show();
}

void optionsWindow::editWebService(int id)
{
	int pos = m_webServicesIds[id];
	auto *wsWindow = new WebServiceWindow(&m_webServices[pos], this);
	connect(wsWindow, &WebServiceWindow::validated, this, &optionsWindow::setWebService);
	wsWindow->show();
}

void optionsWindow::removeWebService(int id)
{
	m_webServices.removeAt(m_webServicesIds[id]);

	// Delete favicon file
	QFile(savePath("webservices/") + QString::number(id) + ".ico").remove();

	// Remove WebService config
	QSettings *settings = m_profile->getSettings();
	settings->beginGroup("WebServices");
	settings->beginGroup(QString::number(id));
	settings->remove("");
	settings->endGroup();
	settings->endGroup();

	showWebServices();
}

void optionsWindow::setWebService(ReverseSearchEngine rse, const QByteArray &favicon)
{
	const bool isNew = rse.id() < 0;

	// Generate new ID for new web services
	if (isNew)
	{
		int maxOrder = 0;
		int maxId = 0;
		for (const ReverseSearchEngine &ws : m_webServices)
		{
			if (ws.id() > maxId)
				maxId = ws.id();
			if (ws.order() > maxOrder)
				maxOrder = ws.order();
		}

		rse.setId(maxId + 1);
		rse.setOrder(maxOrder + 1);
	}

	// Write icon information to disk
	if (!favicon.isEmpty())
	{
		QString faviconPath = savePath("webservices/") + QString::number(rse.id()) + ".ico";
		QFile f(faviconPath);
		if (f.open(QFile::WriteOnly))
		{
			f.write(favicon);
			f.close();
		}
		rse = ReverseSearchEngine(rse.id(), faviconPath, rse.name(), rse.tpl(), rse.order());
	}

	if (isNew)
	{ m_webServices.append(rse); }
	else
	{ m_webServices[m_webServicesIds[rse.id()]] = rse; }

	showWebServices();
}

void optionsWindow::moveUpWebService(int id)
{
	const int i = m_webServicesIds[id];
	if (i == 0)
		return;

	swapWebServices(i, i - 1);
}

void optionsWindow::moveDownWebService(int id)
{
	const int i = m_webServicesIds[id];
	if (i == m_webServicesIds.count() - 1)
		return;

	swapWebServices(i, i + 1);
}

bool sortByOrder(const ReverseSearchEngine &a, const ReverseSearchEngine &b)
{ return a.order() < b.order(); }
void optionsWindow::swapWebServices(int a, int b)
{
	const int pos = m_webServices[b].order();
	m_webServices[b].setOrder(m_webServices[a].order());
	m_webServices[a].setOrder(pos);

	// Re-order web services
	std::sort(m_webServices.begin(), m_webServices.end(), sortByOrder);
	m_webServicesIds.clear();
	for (int i = 0; i < m_webServices.count(); ++i)
		m_webServicesIds.insert(m_webServices[i].id(), i);

	showWebServices();
}


void optionsWindow::setColor(QLineEdit *lineEdit, bool button)
{
	const QString text = lineEdit->text();
	QColor color = button
		? QColorDialog::getColor(QColor(text), this, tr("Choose a color"))
		: QColor(text);

	if (color.isValid())
	{
		lineEdit->setText(button ? color.name() : text);
		lineEdit->setStyleSheet("color:" + color.name());
	}
	else if (!button)
	{ lineEdit->setStyleSheet("color:#000000"); }
}

void optionsWindow::setFont(QLineEdit *lineEdit)
{
	bool ok = false;
	const QFont police = QFontDialog::getFont(&ok, lineEdit->font(), this, tr("Choose a font"));

	if (ok)
	{ lineEdit->setFont(police); }
}

void optionsWindow::on_lineColoringArtists_textChanged()
{ setColor(ui->lineColoringArtists); }
void optionsWindow::on_lineColoringCircles_textChanged()
{ setColor(ui->lineColoringCircles); }
void optionsWindow::on_lineColoringCopyrights_textChanged()
{ setColor(ui->lineColoringCopyrights); }
void optionsWindow::on_lineColoringCharacters_textChanged()
{ setColor(ui->lineColoringCharacters); }
void optionsWindow::on_lineColoringSpecies_textChanged()
{ setColor(ui->lineColoringSpecies); }
void optionsWindow::on_lineColoringMetas_textChanged()
{ setColor(ui->lineColoringMetas); }
void optionsWindow::on_lineColoringModels_textChanged()
{ setColor(ui->lineColoringModels); }
void optionsWindow::on_lineColoringGenerals_textChanged()
{ setColor(ui->lineColoringGenerals); }
void optionsWindow::on_lineColoringFavorites_textChanged()
{ setColor(ui->lineColoringFavorites); }
void optionsWindow::on_lineColoringKeptForLater_textChanged()
{ setColor(ui->lineColoringKeptForLater); }
void optionsWindow::on_lineColoringBlacklisteds_textChanged()
{ setColor(ui->lineColoringBlacklisteds); }
void optionsWindow::on_lineColoringIgnoreds_textChanged()
{ setColor(ui->lineColoringIgnoreds); }
void optionsWindow::on_lineBorderColor_textChanged()
{ setColor(ui->lineBorderColor); }

void optionsWindow::on_buttonColoringArtistsColor_clicked()
{ setColor(ui->lineColoringArtists, true); }
void optionsWindow::on_buttonColoringCirclesColor_clicked()
{ setColor(ui->lineColoringCircles, true); }
void optionsWindow::on_buttonColoringCopyrightsColor_clicked()
{ setColor(ui->lineColoringCopyrights, true); }
void optionsWindow::on_buttonColoringCharactersColor_clicked()
{ setColor(ui->lineColoringCharacters, true); }
void optionsWindow::on_buttonColoringSpeciesColor_clicked()
{ setColor(ui->lineColoringSpecies, true); }
void optionsWindow::on_buttonColoringMetasColor_clicked()
{ setColor(ui->lineColoringMetas, true); }
void optionsWindow::on_buttonColoringModelsColor_clicked()
{ setColor(ui->lineColoringModels, true); }
void optionsWindow::on_buttonColoringGeneralsColor_clicked()
{ setColor(ui->lineColoringGenerals, true); }
void optionsWindow::on_buttonColoringFavoritesColor_clicked()
{ setColor(ui->lineColoringFavorites, true); }
void optionsWindow::on_buttonColoringKeptForLaterColor_clicked()
{ setColor(ui->lineColoringKeptForLater, true); }
void optionsWindow::on_buttonColoringBlacklistedsColor_clicked()
{ setColor(ui->lineColoringBlacklisteds, true); }
void optionsWindow::on_buttonColoringIgnoredsColor_clicked()
{ setColor(ui->lineColoringIgnoreds, true); }
void optionsWindow::on_buttonBorderColor_clicked()
{ setColor(ui->lineBorderColor, true); }

void optionsWindow::on_buttonColoringArtistsFont_clicked()
{ setFont(ui->lineColoringArtists); }
void optionsWindow::on_buttonColoringCirclesFont_clicked()
{ setFont(ui->lineColoringCircles); }
void optionsWindow::on_buttonColoringCopyrightsFont_clicked()
{ setFont(ui->lineColoringCopyrights); }
void optionsWindow::on_buttonColoringCharactersFont_clicked()
{ setFont(ui->lineColoringCharacters); }
void optionsWindow::on_buttonColoringSpeciesFont_clicked()
{ setFont(ui->lineColoringSpecies); }
void optionsWindow::on_buttonColoringMetasFont_clicked()
{ setFont(ui->lineColoringMetas); }
void optionsWindow::on_buttonColoringModelsFont_clicked()
{ setFont(ui->lineColoringModels); }
void optionsWindow::on_buttonColoringGeneralsFont_clicked()
{ setFont(ui->lineColoringGenerals); }
void optionsWindow::on_buttonColoringFavoritesFont_clicked()
{ setFont(ui->lineColoringFavorites); }
void optionsWindow::on_buttonColoringKeptForLaterFont_clicked()
{ setFont(ui->lineColoringKeptForLater); }
void optionsWindow::on_buttonColoringBlacklistedsFont_clicked()
{ setFont(ui->lineColoringBlacklisteds); }
void optionsWindow::on_buttonColoringIgnoredsFont_clicked()
{ setFont(ui->lineColoringIgnoreds); }

void optionsWindow::on_lineImageBackgroundColor_textChanged()
{ setColor(ui->lineImageBackgroundColor); }
void optionsWindow::on_buttonImageBackgroundColor_clicked()
{ setColor(ui->lineImageBackgroundColor, true); }

void treeWidgetRec(int depth, bool &found, int &index, QTreeWidgetItem *current, QTreeWidgetItem *sel)
{
	if (current == sel)
	{
		found = true;
		return;
	}
	index++;

	for (int i = 0; i < current->childCount(); ++i)
	{
		treeWidgetRec(depth + 1, found, index, current->child(i), sel);
		if (found)
			break;
	}
}

void optionsWindow::updateContainer(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	Q_UNUSED(previous);

	bool found = false;
	int index = 0;

	for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i)
	{
		treeWidgetRec(0, found, index, ui->treeWidget->topLevelItem(i), current);
		if (found)
			break;
	}

	if (found)
		ui->stackedWidget->setCurrentIndex(index);
}

void optionsWindow::save()
{
	QSettings *settings = m_profile->getSettings();

	settings->setValue("whitelistedtags", ui->lineWhitelist->text());
	settings->setValue("ignoredtags", ui->lineIgnored->text());
	settings->setValue("add", ui->lineAdd->text());
	QStringList wl = QStringList() << "never" << "image" << "page";
	settings->setValue("whitelist_download", wl.at(ui->comboWhitelist->currentIndex()));

	settings->setValue("limit", ui->spinImagesPerPage->value());
	settings->setValue("columns", ui->spinColumns->value());
	QStringList sources = QStringList() << "xml" << "json" << "regex" << "rss";
	settings->setValue("source_1", sources.at(ui->comboSource1->currentIndex()));
	settings->setValue("source_2", sources.at(ui->comboSource2->currentIndex()));
	settings->setValue("source_3", sources.at(ui->comboSource3->currentIndex()));
	settings->setValue("source_4", sources.at(ui->comboSource4->currentIndex()));
	settings->setValue("tagsautoadd", ui->spinAutoTagAdd->value());
	QStringList starts = QStringList() << "none" << "loadfirst" << "restore";
	settings->setValue("start", starts.at(ui->comboStart->currentIndex()));
	settings->setValue("hidefavorites", ui->spinHideFavorites->value());
	settings->setValue("autodownload", ui->checkAutodownload->isChecked());
	settings->setValue("hideblacklisted", ui->checkHideBlacklisted->isChecked());
	settings->setValue("showtagwarning", ui->checkShowTagWarning->isChecked());
	settings->setValue("showwarnings", ui->checkShowWarnings->isChecked());
	settings->setValue("getunloadedpages", ui->checkGetUnloadedPages->isChecked());
	settings->setValue("invertToggle", ui->checkInvertToggle->isChecked());
	settings->setValue("confirm_close", ui->checkConfirmClose->isChecked());
	QList<int> checkForUpdates = QList<int>() << 0 << 24*60*60 << 7*24*60*60 << 30*24*60*60 << -1;
	settings->setValue("check_for_updates", checkForUpdates.at(ui->comboCheckForUpdates->currentIndex()));

	settings->beginGroup("Filenames");
		for (int i = 0; i < m_filenamesConditions.size(); i++)
		{
			if (!m_filenamesConditions.at(i)->text().isEmpty())
			{
				settings->setValue(QString::number(i) + "_cond", m_filenamesConditions.at(i)->text());
				settings->setValue(QString::number(i) + "_fn", m_filenamesFilenames.at(i)->text());
				settings->setValue(QString::number(i) + "_dir", m_filenamesFolders.at(i)->text());
			}
			else
			{
				settings->remove(QString::number(i) + "_cond");
				settings->remove(QString::number(i) + "_fn");
				settings->remove(QString::number(i) + "_dir");
			}
		}
	settings->endGroup();

	QStringList types = QStringList() << "text" << "icon" << "both" << "hide";
	settings->setValue("Sources/Types", types.at(ui->comboSources->currentIndex()));
	const int i = ui->comboSourcesLetters->currentIndex();
	settings->setValue("Sources/Letters", (i == 0 ? ui->spinSourcesLetters->value() : -i));
	settings->setValue("preloadAllTabs", ui->checkPreloadAllTabs->isChecked());

	QStringList ftypes = QStringList() << "ind" << "in" << "id" << "nd" << "i" << "n" << "d";
	if (settings->value("favorites_display", "ind").toString() != ftypes.at(ui->comboFavoritesDisplay->currentIndex()))
	{
		settings->setValue("favorites_display", ftypes.at(ui->comboFavoritesDisplay->currentIndex()));
		m_profile->emitFavorite();
	}

	// Log
	settings->beginGroup("Log");
		settings->setValue("show", ui->checkShowLog->isChecked());
	settings->endGroup();

	// Blacklist
	Blacklist blacklist;
	for (const QString &tags : ui->textBlacklist->toPlainText().split("\n", QString::SkipEmptyParts))
	{ blacklist.add(tags.trimmed().split(' ', QString::SkipEmptyParts)); }
	m_profile->setBlacklistedTags(blacklist);
	settings->setValue("downloadblacklist", ui->checkDownloadBlacklisted->isChecked());

	// Monitoring
	settings->beginGroup("Monitoring");
		settings->setValue("startupDelay", ui->spinMonitoringStartupDelay->value());
		settings->setValue("enableTray", ui->checkMonitoringEnableTray->isChecked());
		settings->setValue("minimizeToTray", ui->checkMonitoringMinimizeToTray->isChecked());
		settings->setValue("closeToTray", ui->checkMonitoringCloseToTray->isChecked());
	settings->endGroup();

	settings->setValue("resizeInsteadOfCropping", ui->checkResizeInsteadOfCropping->isChecked());
	settings->setValue("thumbnailUpscale", static_cast<double>(ui->spinThumbnailUpscale->value()) / 100.0);
	settings->setValue("autocompletion", ui->checkAutocompletion->isChecked());
	settings->setValue("useregexfortags", ui->checkUseregexfortags->isChecked());
	QStringList infiniteScroll = QStringList() << "disabled" << "button" << "scroll";
	settings->setValue("infiniteScroll", infiniteScroll.at(ui->comboInfiniteScroll->currentIndex()));
	settings->setValue("infiniteScrollRememberPage", ui->checkInfiniteScrollRememberPage->isChecked());

	settings->setValue("Batch/end", ui->comboBatchEnd->currentIndex());
	settings->beginGroup("Save");
		settings->setValue("automaticretries", ui->spinAutomaticRetries->value());
		settings->setValue("downloadoriginals", ui->checkDownloadOriginals->isChecked());
		settings->setValue("samplefallback", ui->checkSampleFallback->isChecked());
		settings->setValue("replaceblanks", ui->checkReplaceBlanks->isChecked());
		settings->setValue("keepDate", ui->checkKeepDate->isChecked());
		settings->setValue("headerDetection", ui->checkSaveHeaderDetection->isChecked());
		settings->setValue("separator", ui->lineSeparator->text());
		settings->setValue("noJpeg", ui->checkNoJpeg->isChecked());
		QString folder = fixFilename("", ui->lineFolder->text());
		settings->setValue("path", folder);
		settings->setValue("path_real", folder);
		QDir pth = QDir(folder);
		if (!pth.exists())
		{
			QString op;
			while (!pth.exists() && pth.path() != op)
			{
				op = pth.path();
				pth.setPath(pth.path().remove(QRegularExpression("/([^/]+)$")));
			}
			if (pth.path() == op)
			{ error(this, tr("An error occured creating the save folder.")); }
			else
			{ pth.mkpath(folder); }
		}
		folder = fixFilename("", ui->lineFolderFavorites->text());
		settings->setValue("path_favorites", folder);
		pth = QDir(folder);
		if (!pth.exists())
		{
			QString op;
			while (!pth.exists() && pth.path() != op)
			{
				op = pth.path();
				pth.setPath(pth.path().remove(QRegularExpression("/([^/]+)$")));
			}
			if (pth.path() == op)
			{ error(this, tr("An error occured creating the favorites save folder.")); }
			else
			{ pth.mkpath(folder); }
		}
		QStringList opts = QStringList() << "save" << "copy" << "move" << "ignore";
		settings->setValue("md5Duplicates", opts.at(ui->comboMd5Duplicates->currentIndex()));
		settings->setValue("keepDeletedMd5", ui->checkKeepDeletedMd5->isChecked());

		settings->setValue("filename", ui->lineFilename->text());
		settings->setValue("filename_real", ui->lineFilename->text());
		settings->setValue("filename_favorites", ui->lineFavorites->text());

		settings->setValue("artist_empty", ui->lineArtistsIfNone->text());
		settings->setValue("artist_useall", ui->radioArtistsKeepAll->isChecked());
		QString artistMultiple;
		if		(ui->radioArtistsKeepAll->isChecked())		{ artistMultiple = "keepAll";		}
		else if	(ui->radioArtistsKeepN->isChecked())		{ artistMultiple = "keepN";			}
		else if	(ui->radioArtistsKeepNThenAdd->isChecked())	{ artistMultiple = "keepNThenAdd";	}
		else if	(ui->radioArtistsReplaceAll->isChecked())	{ artistMultiple = "replaceAll";	}
		else if	(ui->radioArtistsMultiple->isChecked())		{ artistMultiple = "multiple";		}
		settings->setValue("artist_multiple", artistMultiple);
		settings->setValue("artist_multiple_limit", ui->spinArtistsMoreThanN->value());
		settings->setValue("artist_multiple_keepN", ui->spinArtistsKeepN->value());
		settings->setValue("artist_multiple_keepNThenAdd_keep", ui->spinArtistsKeepNThenAdd->value());
		settings->setValue("artist_multiple_keepNThenAdd_add", ui->lineArtistsKeepNThenAdd->text());
		settings->setValue("artist_sep", ui->lineArtistsSeparator->text());
		settings->setValue("artist_value", ui->lineArtistsReplaceAll->text());

		settings->setValue("copyright_empty", ui->lineCopyrightsIfNone->text());
		settings->setValue("copyright_useshorter", ui->checkCopyrightsUseShorter->isChecked());
		QString copyrightMultiple;
		if		(ui->radioCopyrightsKeepAll->isChecked())		{ copyrightMultiple = "keepAll";		}
		else if	(ui->radioCopyrightsKeepN->isChecked())			{ copyrightMultiple = "keepN";			}
		else if	(ui->radioCopyrightsKeepNThenAdd->isChecked())	{ copyrightMultiple = "keepNThenAdd";	}
		else if	(ui->radioCopyrightsReplaceAll->isChecked())	{ copyrightMultiple = "replaceAll";		}
		else if	(ui->radioCopyrightsMultiple->isChecked())		{ copyrightMultiple = "multiple";		}
		settings->setValue("copyright_multiple", copyrightMultiple);
		settings->setValue("copyright_multiple_limit", ui->spinCopyrightsMoreThanN->value());
		settings->setValue("copyright_multiple_keepN", ui->spinCopyrightsKeepN->value());
		settings->setValue("copyright_multiple_keepNThenAdd_keep", ui->spinCopyrightsKeepNThenAdd->value());
		settings->setValue("copyright_multiple_keepNThenAdd_add", ui->lineCopyrightsKeepNThenAdd->text());
		settings->setValue("copyright_sep", ui->lineCopyrightsSeparator->text());
		settings->setValue("copyright_value", ui->lineCopyrightsReplaceAll->text());

		settings->setValue("character_empty", ui->lineCharactersIfNone->text());
		QString characterMultiple;
		if		(ui->radioCharactersKeepAll->isChecked())		{ characterMultiple = "keepAll";		}
		else if	(ui->radioCharactersKeepN->isChecked())			{ characterMultiple = "keepN";			}
		else if	(ui->radioCharactersKeepNThenAdd->isChecked())	{ characterMultiple = "keepNThenAdd";	}
		else if	(ui->radioCharactersReplaceAll->isChecked())	{ characterMultiple = "replaceAll";		}
		else if	(ui->radioCharactersMultiple->isChecked())		{ characterMultiple = "multiple";		}
		settings->setValue("character_multiple", characterMultiple);
		settings->setValue("character_multiple_limit", ui->spinCharactersMoreThanN->value());
		settings->setValue("character_multiple_keepN", ui->spinCharactersKeepN->value());
		settings->setValue("character_multiple_keepNThenAdd_keep", ui->spinCharactersKeepNThenAdd->value());
		settings->setValue("character_multiple_keepNThenAdd_add", ui->lineCharactersKeepNThenAdd->text());
		settings->setValue("character_sep", ui->lineCharactersSeparator->text());
		settings->setValue("character_value", ui->lineCharactersReplaceAll->text());

		settings->setValue("species_empty", ui->lineSpeciesIfNone->text());
		QString speciesMultiple;
		if		(ui->radioSpeciesKeepAll->isChecked())		{ speciesMultiple = "keepAll";		}
		else if	(ui->radioSpeciesKeepN->isChecked())		{ speciesMultiple = "keepN";		}
		else if	(ui->radioSpeciesKeepNThenAdd->isChecked())	{ speciesMultiple = "keepNThenAdd";	}
		else if	(ui->radioSpeciesReplaceAll->isChecked())	{ speciesMultiple = "replaceAll";	}
		else if	(ui->radioSpeciesMultiple->isChecked())		{ speciesMultiple = "multiple";		}
		settings->setValue("species_multiple", speciesMultiple);
		settings->setValue("species_multiple_limit", ui->spinSpeciesMoreThanN->value());
		settings->setValue("species_multiple_keepN", ui->spinSpeciesKeepN->value());
		settings->setValue("species_multiple_keepNThenAdd_keep", ui->spinSpeciesKeepNThenAdd->value());
		settings->setValue("species_multiple_keepNThenAdd_add", ui->lineSpeciesKeepNThenAdd->text());
		settings->setValue("species_sep", ui->lineSpeciesSeparator->text());
		settings->setValue("species_value", ui->lineSpeciesReplaceAll->text());

		settings->setValue("meta_empty", ui->lineMetasIfNone->text());
		QString metaMultiple;
		if		(ui->radioMetasKeepAll->isChecked())		{ metaMultiple = "keepAll";			}
		else if	(ui->radioMetasKeepN->isChecked())			{ metaMultiple = "keepN";			}
		else if	(ui->radioMetasKeepNThenAdd->isChecked())	{ metaMultiple = "keepNThenAdd";	}
		else if	(ui->radioMetasReplaceAll->isChecked())		{ metaMultiple = "replaceAll";		}
		else if	(ui->radioMetasMultiple->isChecked())		{ metaMultiple = "multiple";		}
		settings->setValue("meta_multiple", metaMultiple);
		settings->setValue("meta_multiple_limit", ui->spinMetasMoreThanN->value());
		settings->setValue("meta_multiple_keepN", ui->spinMetasKeepN->value());
		settings->setValue("meta_multiple_keepNThenAdd_keep", ui->spinMetasKeepNThenAdd->value());
		settings->setValue("meta_multiple_keepNThenAdd_add", ui->lineMetasKeepNThenAdd->text());
		settings->setValue("meta_sep", ui->lineMetasSeparator->text());
		settings->setValue("meta_value", ui->lineMetasReplaceAll->text());

		settings->setValue("limit", ui->spinLimit->value());
		settings->setValue("simultaneous", ui->spinSimultaneous->value());
		settings->beginGroup("Customs");
			settings->remove("");
			for (int j = 0; j < m_customNames.size(); j++)
			{ settings->setValue(m_customNames[j]->text(), m_customTags[j]->text()); }
		settings->endGroup();
	settings->endGroup();

	// Web services
	settings->beginGroup("WebServices");
	for (const ReverseSearchEngine &webService : m_webServices)
	{
		settings->beginGroup(QString::number(webService.id()));
		settings->setValue("name", webService.name());
		settings->setValue("url", webService.tpl());
		settings->setValue("order", webService.order());
		settings->endGroup();
	}
	settings->endGroup();

	// Themes
	const QString theme = ui->comboTheme->currentText();
	ThemeLoader themeLoader(savePath("themes/", true));
	if (themeLoader.setTheme(theme))
	{ settings->setValue("theme", theme); }

	settings->setValue("Zoom/singleWindow", ui->checkSingleDetailsWindow->isChecked());
	QStringList positions = QStringList() << "top" << "left" << "auto";
	settings->setValue("tagsposition", positions.at(ui->comboTagsPosition->currentIndex()));
	settings->setValue("preload", ui->spinPreload->value());
	settings->setValue("slideshow", ui->spinSlideshow->value());
	settings->setValue("resultsScrollArea", ui->checkResultsScrollArea->isChecked());
	settings->setValue("resultsFixedWidthLayout", ui->checkResultsFixedWidthLayout->isChecked());
	settings->setValue("imageCloseMiddleClick", ui->checkImageCloseMiddleClick->isChecked());
	settings->setValue("imageNavigateScroll", ui->checkImageNavigateScroll->isChecked());
	settings->setValue("Zoom/showTagCount", ui->checkZoomShowTagCount->isChecked());
	//settings->setValue("Zoom/viewSamples", ui->checkZoomViewSamples->isChecked());
	QStringList imageTagOrder = QStringList() << "type" << "name" << "count";
	settings->setValue("Zoom/tagOrder", imageTagOrder.at(ui->comboImageTagOrder->currentIndex()));
	QStringList positionsV = QStringList() << "top" << "center" << "bottom";
	QStringList positionsH = QStringList() << "left" << "center" << "right";
	settings->setValue("imagePositionImageV", positionsV.at(ui->comboImagePositionImageV->currentIndex()));
	settings->setValue("imagePositionImageH", positionsH.at(ui->comboImagePositionImageH->currentIndex()));
	settings->setValue("imagePositionAnimationV", positionsV.at(ui->comboImagePositionAnimationV->currentIndex()));
	settings->setValue("imagePositionAnimationH", positionsH.at(ui->comboImagePositionAnimationH->currentIndex()));
	settings->setValue("imagePositionVideoV", positionsV.at(ui->comboImagePositionVideoV->currentIndex()));
	settings->setValue("imagePositionVideoH", positionsH.at(ui->comboImagePositionVideoH->currentIndex()));
	settings->setValue("imageBackgroundColor", ui->lineImageBackgroundColor->text());

	settings->beginGroup("Coloring");
		settings->beginGroup("Colors");
			settings->setValue("artists", ui->lineColoringArtists->text());
			settings->setValue("circles", ui->lineColoringCircles->text());
			settings->setValue("copyrights", ui->lineColoringCopyrights->text());
			settings->setValue("characters", ui->lineColoringCharacters->text());
			settings->setValue("species", ui->lineColoringSpecies->text());
			settings->setValue("metas", ui->lineColoringMetas->text());
			settings->setValue("models", ui->lineColoringModels->text());
			settings->setValue("generals", ui->lineColoringGenerals->text());
			settings->setValue("favorites", ui->lineColoringFavorites->text());
			settings->setValue("keptForLater", ui->lineColoringKeptForLater->text());
			settings->setValue("blacklisteds", ui->lineColoringBlacklisteds->text());
			settings->setValue("ignoreds", ui->lineColoringIgnoreds->text());
		settings->endGroup();
		settings->beginGroup("Fonts");
			settings->setValue("artists", ui->lineColoringArtists->font().toString());
			settings->setValue("circles", ui->lineColoringCircles->font().toString());
			settings->setValue("copyrights", ui->lineColoringCopyrights->font().toString());
			settings->setValue("characters", ui->lineColoringCharacters->font().toString());
			settings->setValue("species", ui->lineColoringSpecies->font().toString());
			settings->setValue("metas", ui->lineColoringMetas->font().toString());
			settings->setValue("models", ui->lineColoringModels->font().toString());
			settings->setValue("generals", ui->lineColoringGenerals->font().toString());
			settings->setValue("favorites", ui->lineColoringFavorites->font().toString());
			settings->setValue("keptForLater", ui->lineColoringKeptForLater->font().toString());
			settings->setValue("blacklisteds", ui->lineColoringBlacklisteds->font().toString());
			settings->setValue("ignoreds", ui->lineColoringIgnoreds->font().toString());
		settings->endGroup();
	settings->endGroup();

	settings->beginGroup("Margins");
		settings->setValue("main", ui->spinMainMargins->value());
		settings->setValue("horizontal", ui->spinHorizontalMargins->value());
		settings->setValue("vertical", ui->spinVerticalMargins->value());
	settings->endGroup();
	settings->setValue("serverBorder", ui->spinServerBorders->value());
	settings->setValue("serverBorderColor", ui->lineBorderColor->text());
	settings->setValue("borders", ui->spinBorders->value());

	settings->beginGroup("Proxy");
		settings->setValue("use", ui->checkProxyUse->isChecked());
		settings->setValue("useSystem", ui->checkProxyUseSystem->isChecked());
		QStringList ptypes = QStringList() << "http" << "socks5";
		settings->setValue("type", ptypes.at(ui->comboProxyType->currentIndex()));
		settings->setValue("hostName", ui->lineProxyHostName->text());
		settings->setValue("port", ui->spinProxyPort->value());
		settings->setValue("user", ui->lineProxyUser->text());
		settings->setValue("password", ui->lineProxyPassword->text());
	settings->endGroup();

	settings->beginGroup("Exec");
		settings->setValue("tag_before", ui->lineCommandsTagAfter->text());
		settings->setValue("image", ui->lineCommandsImage->text());
		settings->setValue("tag_after", ui->lineCommandsTagBefore->text());
		settings->beginGroup("SQL");
			settings->setValue("driver", ui->comboCommandsSqlDriver->currentText());
			settings->setValue("host", ui->lineCommandsSqlHost->text());
			settings->setValue("user", ui->lineCommandsSqlUser->text());
			settings->setValue("password", ui->lineCommandsSqlPassword->text());
			settings->setValue("database", ui->lineCommandsSqlDatabase->text());
			settings->setValue("before", ui->lineCommandsSqlBefore->text());
			settings->setValue("tag_before", ui->lineCommandsSqlTagBefore->text());
			settings->setValue("image", ui->lineCommandsSqlImage->text());
			settings->setValue("tag_after", ui->lineCommandsSqlTagAfter->text());
			settings->setValue("after", ui->lineCommandsSqlAfter->text());
		settings->endGroup();
	settings->endGroup();

	if (settings->value("Proxy/use", false).toBool())
	{
		const bool useSystem = settings->value("Proxy/useSystem", false).toBool();
		QNetworkProxyFactory::setUseSystemConfiguration(useSystem);

		if (!useSystem)
		{
			const QNetworkProxy::ProxyType type = settings->value("Proxy/type", "http") == "http"
				? QNetworkProxy::HttpProxy
				: QNetworkProxy::Socks5Proxy;
			const QNetworkProxy proxy(
				type,
				settings->value("Proxy/hostName").toString(),
				settings->value("Proxy/port").toInt(),
				settings->value("Proxy/user").toString(),
				settings->value("Proxy/password").toString()
			);
			QNetworkProxy::setApplicationProxy(proxy);
			log(QStringLiteral("Enabling application proxy on host \"%1\" and port %2.").arg(settings->value("Proxy/hostName").toString()).arg(settings->value("Proxy/port").toInt()));
		}
		else
		{ log(QStringLiteral("Enabling system-wide proxy.")); }
	}
	else if (QNetworkProxy::applicationProxy().type() != QNetworkProxy::NoProxy)
	{
		QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
		log(QStringLiteral("Disabling application proxy."));
	}

	const QString lang = ui->comboLanguages->currentData().toString();
	if (settings->value("language", "English").toString() != lang)
	{
		settings->setValue("language", lang);
		emit languageChanged(lang);
	}

	m_profile->sync();
	emit settingsChanged();
}
