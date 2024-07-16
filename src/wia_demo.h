
#pragma once

#include <QLibrary>
#include <QScopedPointer>
#include <QDialog>
#include <QAxObject>
#include <QDebug>

#include "ui_wia_demo.h"



class WiaDemoDialog : public QDialog
{
    Q_OBJECT
public:
    WiaDemoDialog(QWidget* parent = nullptr);
    ~WiaDemoDialog();

protected:
    void timerEvent(QTimerEvent* event) {
        // It's for visual detection of blocking operations
        Q_UNUSED(event);
        qDebug() << ".";
    }

private slots:
    void onException(int code, const QString& source, const QString& desc, const QString& help) {
        qDebug() << "WiaDemoDialog::onException" << code << source << desc << help;
    }

    void slotGetDevices();
    void slotScan();

    void slot_exception(int code, const QString& source, const QString& desc, const QString& help);
    void slot_propertyChanged(const QString& name);
    void slot_signal(const QString& name, int argc, void* argv);

private:
    void getDevices();
    void scan() noexcept(false);
    QStringList scanDocument(QAxObject* device, QAxObject* scanner) noexcept(false);
    void printObjectInfo(QAxObject*, const QString& fileName = "Doc.html");
    void printObjectProperties(QAxObject*) const;
    QString deviceTypeToString(int) const;
    void setWIAProperty(QAxObject* properties, const QString& name, const QVariant& value);
    QVariantMap getWIAProperties(QAxObject* properties);
    void adjustScannerSettings(QAxObject* deviceProps, QAxObject* scannerItem);
    QString generateIncrementedFileName(const QString& name, const QString& extension) const;
    void buildPdf(const QStringList& pages);

    QSharedPointer<QAxObject> createSubObject(QAxObject* parentObj, const char* subObjName, const QList<QVariant>& vars = QVariantList())  noexcept(false);
    QSharedPointer<QAxObject> createSubObject(QAxObject* parentObj, const char* subObjName, const QVariant& var)  noexcept(false)
    { 
        return createSubObject(parentObj, subObjName, QList<QVariant>() << var);
    };

private:


    QAxObject* wiaDevMgr = nullptr;
    Ui::wia ui;
    QString m_lastErrorString;
    int m_lastErrorCode = 0;

    // WIA constants

    const QString wiaFormatBMP = "{B96B3CAB-0728-11D3-9D7B-0000F81EF32E}";
    const QString wiaFormatPNG = "{B96B3CAF-0728-11D3-9D7B-0000F81EF32E}";
    const QString wiaFormatGIF = "{B96B3CB0-0728-11D3-9D7B-0000F81EF32E}";
    const QString wiaFormatJPEG = "{B96B3CAE-0728-11D3-9D7B-0000F81EF32E}";
    const QString wiaFormatTIFF = "{B96B3CB1-0728-11D3-9D7B-0000F81EF32E}";

    enum WIA_DIP_DEV_TYPE {
        StiDeviceTypeDefault = 0,
        StiDeviceTypeScanner = 1,
        StiDeviceTypeDigitalCamera = 2,
        StiDeviceTypeStreamingVideo = 3,
    };

    enum WIA_DPS_DOCUMENT_HANDLING_SELECT {
        WIA_FEEDER = 0x001,
        WIA_FLATBED = 0x002,
        WIA_DUPLEX = 0x004,
        WIA_FRONT_FIRST = 0x008,
        WIA_BACK_FIRST = 0x010,
        WIA_FRONT_ONLY = 0x020,
        WIA_BACK_ONLY = 0x040,
        WIA_NEXT_PAGE = 0x080,
        WIA_PREFEED = 0x100,
        WIA_AUTO_ADVANCE = 0x200
    };

    enum WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES {
        WIA_FEED = 0x01,
        WIA_FLAT = 0x02,
        WIA_DUP = 0x04,
        WIA_DETECT_FLAT = 0x08,
        WIA_DETECT_SCAN = 0x10,
        WIA_DETECT_FEED = 0x20,
        WIA_DETECT_DUP = 0x40,
        WIA_DETECT_FEED_AVAIL = 0x80,
        WIA_DETECT_DUP_AVAIL = 0x100,
    };

    enum WIA_DPS_DOCUMENT_HANDLING_STATUS {
        WIA_FEED_READY = 0x01,
        WIA_FLAT_READY = 0x02,
        WIA_DUP_READY = 0x04,
        WIA_FLAT_COVER_UP = 0x08,
        WIA_PATH_COVER_UP = 0x10,
        WIA_PAPER_JAM = 0x20
    };

    enum WIA_ERROR_CODES {
        WIA_ERROR_BUSY = 0x80210006,
        WIA_ERROR_COVER_OPEN = 0x80210016,
        WIA_ERROR_DEVICE_COMMUNICATION = 0x8021000A,
        WIA_ERROR_DEVICE_LOCKED = 0x8021000D,
        WIA_ERROR_EXCEPTION_IN_DRIVER = 0x8021000E,
        WIA_ERROR_GENERAL_ERROR = 0x80210001,
        WIA_ERROR_INCORRECT_HARDWARE_SETTING = 0x8021000C,
        WIA_ERROR_INVALID_COMMAND = 0x8021000B,
        WIA_ERROR_INVALID_DRIVER_RESPONSE = 0x8021000F,
        WIA_ERROR_ITEM_DELETED = 0x80210009,
        WIA_ERROR_LAMP_OFF = 0x80210017,
        WIA_ERROR_MAXIMUM_PRINTER_ENDORSER_COUNTER = 0x80210021,
        WIA_ERROR_MULTI_FEED = 0x80210020,
        WIA_ERROR_OFFLINE = 0x80210005,
        WIA_ERROR_PAPER_EMPTY = 0x80210003,
        WIA_ERROR_PAPER_JAM = 0x80210002,
        WIA_ERROR_PAPER_PROBLEM = 0x80210004,
        WIA_ERROR_WARMING_UP = 0x80210007,
        WIA_ERROR_USER_INTERVENTION = 0x80210008,
        WIA_S_NO_DEVICE_AVAILABLE = 0x80210015
    };
};
