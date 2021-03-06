#include "mixed-settings.h"
#include <QSettings>
#include "mixed-settings-test.h"


void MixedSettingsTest::init()
{
	m_child = new QSettings("tests/resources/tmp/child.ini", QSettings::IniFormat);
	m_parent = new QSettings("tests/resources/tmp/parent.ini", QSettings::IniFormat);

	m_child->clear();
	m_parent->clear();
}

void MixedSettingsTest::testValueFirstValid()
{
	MixedSettings settings(QList<QSettings*>() << m_child << m_parent);

	m_child->setValue("test", "child");
	m_parent->setValue("test", "parent");
	QVariant v1 = m_child->value("test");
	QVariant v2 = m_child->value("test");

	QCOMPARE(settings.value("test").toString(), QString("child"));
}

void MixedSettingsTest::testValueDefault()
{
	MixedSettings settings(QList<QSettings*>() << m_child << m_parent);

	QCOMPARE(settings.value("test", "default").toString(), QString("default"));
}

void MixedSettingsTest::testSetValueResetToParent()
{
	MixedSettings settings(QList<QSettings*>() << m_child << m_parent);

	m_child->setValue("test", "child");
	m_parent->setValue("test", "parent");

	settings.setValue("test", "parent", "default");
	settings.sync();

	QCOMPARE(m_child->value("test", "none").toString(), QString("none"));
	QCOMPARE(m_parent->value("test", "none").toString(), QString("parent"));
}

void MixedSettingsTest::testSetValueResetToDefault()
{
	MixedSettings settings(QList<QSettings*>() << m_child << m_parent);

	m_child->setValue("test", "child");

	settings.setValue("test", "default", "default");
	settings.sync();

	QCOMPARE(m_child->value("test", "none").toString(), QString("none"));
	QCOMPARE(m_parent->value("test", "none").toString(), QString("none"));
}

void MixedSettingsTest::testSetValueOverrideParent()
{
	MixedSettings settings(QList<QSettings*>() << m_child << m_parent);

	m_parent->setValue("test", "parent");

	settings.setValue("test", "child", "default");
	settings.sync();

	QCOMPARE(m_child->value("test", "none").toString(), QString("child"));
	QCOMPARE(m_parent->value("test", "none").toString(), QString("parent"));
}

void MixedSettingsTest::testSetValueOverrideDefault()
{
	MixedSettings settings(QList<QSettings*>() << m_child << m_parent);

	settings.setValue("test", "child", "default");
	settings.sync();

	QCOMPARE(m_child->value("test", "none").toString(), QString("child"));
	QCOMPARE(m_parent->value("test", "none").toString(), QString("none"));
}

void MixedSettingsTest::testChildKeys()
{
	MixedSettings settings(QList<QSettings*>() << m_child << m_parent);

	m_child->setValue("child", "value");
	m_parent->setValue("parent", "value");

	QCOMPARE(settings.childKeys(), QStringList() << "child" << "parent");
}


QTEST_MAIN(MixedSettingsTest)
