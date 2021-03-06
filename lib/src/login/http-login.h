#ifndef HTTP_LOGIN_H
#define HTTP_LOGIN_H

#include <QNetworkReply>
#include <QSettings>
#include "login/login.h"


class CustomNetworkAccessManager;
class MixedSettings;
class Site;

class HttpLogin : public Login
{
	Q_OBJECT

	public:
		explicit HttpLogin(QString type, Site *site, CustomNetworkAccessManager *manager, MixedSettings *settings);
		virtual ~HttpLogin() = default;
		bool isTestable() const override;
		virtual QNetworkReply *getReply(const QString &url, const QUrlQuery &query) const = 0;

	public slots:
		void login() override;

	protected slots:
		void loginFinished();

	protected:
		QString m_type;
		Site *m_site;
		QNetworkReply *m_loginReply;
		CustomNetworkAccessManager *m_manager;
		MixedSettings *m_settings;
};

#endif // HTTP_LOGIN_H
