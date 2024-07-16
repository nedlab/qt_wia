
#include "wia_demo.h"

#include <QDialog>
#include <QDebug>
#include <QAxObject>
#include <QStringListModel>
#include <QListWidgetItem>
#include <QMetaObject>
#include <QMetaProperty> 
#include <QMessageBox>
#include <QPainter>
#include <QPrinter>
#include <QFile>
#include <QFileInfo>

#include <stdexcept>


/// WiaDemoDialog
WiaDemoDialog::WiaDemoDialog(QWidget* parent) :
    QDialog(parent),
    wiaDevMgr(new QAxObject(this))
{
    startTimer(1000);
    if (!wiaDevMgr->setControl("WIA.DeviceManager")) {
        qDebug() << "WARNING: qaxbase setcontrol failed";
    }

    connect(wiaDevMgr, SIGNAL(exception(int, QString, QString, QString)), this, SLOT(onException(int, QString, QString, QString)));

    ui.setupUi(this);

    connect(ui.pbGetDevices, &QPushButton::clicked, this, &WiaDemoDialog::slotGetDevices);

    connect(ui.devicesView, &QListWidget::itemSelectionChanged, this, [this]() {
        ui.pbScan->setEnabled(ui.devicesView->selectedItems().length() > 0);
    });

    connect(ui.pbScan, &QPushButton::clicked, this, &WiaDemoDialog::slotScan);
}

WiaDemoDialog::~WiaDemoDialog()
{
    delete wiaDevMgr;
}

void WiaDemoDialog::slotGetDevices()
{
    try {
        getDevices();
    }
    catch (const std::exception& ex) {
        QMessageBox::warning(this, "Error", QString::fromUtf8(ex.what()));
    }
}

void WiaDemoDialog::getDevices()
{
    ui.devicesView->clear();
    auto wiaDevices = createSubObject(wiaDevMgr, "DeviceInfos()");
    //    printObjectInfo(wiaDevices, "DeviceInfos.html");

    int deviceCount = wiaDevices->property("Count").toInt();
    ui.devicesCount->setText(QString("Devices count: %1").arg(deviceCount));
    qDebug() << "Number of WIA devices found:" << deviceCount;

    for (int i = 1; i <= deviceCount; ++i) {
        // fill device list
        auto wiaDevice = createSubObject(wiaDevices.data(), "Item(QVariant&)", i);
        auto listItem = new QListWidgetItem(ui.devicesView);
        qDebug() << "Device type:" << deviceTypeToString(wiaDevice->property("Type").toInt());
        printObjectProperties(wiaDevice.data());
        listItem->setData(Qt::UserRole, wiaDevice->property("DeviceID"));
        printObjectInfo(wiaDevice.data(), "Item.html");
        auto wiaDeviceProps = createSubObject(wiaDevice.data(), "Properties");
        auto props = getWIAProperties(wiaDeviceProps.data());
        listItem->setText(props["Description"].toString());
        qDebug() << "Device properties:" << props;
        ui.devicesView->addItem(listItem);
        // fill document source
        auto wiaDeviceExt = createSubObject(wiaDevice.data(), "Connect()");
        auto wiaDeviceExtProps = createSubObject(wiaDeviceExt.data(), "Properties");
        props = getWIAProperties(wiaDeviceExtProps.data());
        qDebug() << "Device ext properties:" << props;
        ui.cbSource->clear();
        if (props["Document Handling Capabilities"].toUInt() & WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES::WIA_FLAT)
            ui.cbSource->addItem("Flatbed", WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES::WIA_FLAT);

        if (props["Document Handling Capabilities"].toUInt() & WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES::WIA_FEED)
            ui.cbSource->addItem("Feeder (ADF)", WIA_DPS_DOCUMENT_HANDLING_CAPABILITIES::WIA_FEED);
    }
}

void WiaDemoDialog::slotScan()
{
    try {
        scan();
    }
    catch (const std::exception& ex) {
        QMessageBox::warning(this, "Error", QString::fromUtf8(ex.what()));
    }
}

void WiaDemoDialog::scan() noexcept(false)
{
    qDebug() << "WiaDemoDialog::scan";
    auto listItem = ui.devicesView->currentItem();
    if (!listItem)
        throw std::logic_error(qUtf8Printable(tr("Scanner is not selected!")));

    auto wiaDevices = createSubObject(wiaDevMgr, "DeviceInfos()");
    auto wiaDevice = createSubObject(wiaDevices.data(), "Item(QVariant&)", listItem->data(Qt::UserRole));
    auto wiaDeviceWorker = createSubObject(wiaDevice.data(), "Connect()");
    auto wiaDeviceWorkerProps = createSubObject(wiaDeviceWorker.data(), "Properties");
    auto wiaDwiaDeviceWorkerItems = createSubObject(wiaDeviceWorker.data(), "Items");
    auto wiaScanItem = createSubObject(wiaDwiaDeviceWorkerItems.data(), "Item(int)", 1);
    
    qDebug() << "Worker props" << getWIAProperties(wiaDeviceWorkerProps.data());

    adjustScannerSettings(wiaDeviceWorker.data(), wiaScanItem.data());
    auto pages = scanDocument(wiaDeviceWorker.data(), wiaScanItem.data());
    if (ui.pdf->isChecked())
        buildPdf(pages);
}

QStringList WiaDemoDialog::scanDocument(QAxObject* device, QAxObject* scanner) noexcept(false)
{
    if (!scanner || !device)
        throw std::logic_error(qUtf8Printable(tr("Scanner is not selected!")));

    bool hasMorePages = false;
    QList<QString> pages;

    do {

        hasMorePages = false;

        // Perform scan
    //    auto wiaImage = createSubObject(scanner, "Transfer(QString)", wiaFormatBMP);
        auto result = scanner->dynamicCall("Transfer(QString)", wiaFormatBMP); // use 'dynamicCall' because we can handle exceptions

        if (WIA_ERROR_CODES::WIA_ERROR_PAPER_EMPTY == m_lastErrorCode && pages.length() > 0)    // detect if feeder become empty
            break;
        else if (!m_lastErrorString.isEmpty())
            throw std::runtime_error(qUtf8Printable(m_lastErrorString));
        auto wiaImage = qobject_cast<QAxObject*>(result.value<QObject*>());
        auto fileData = createSubObject(wiaImage/*.data()*/, "FileData");

        auto img = QImage::fromData(fileData->property("BinaryData").toByteArray(), "BMP");
        if (img.isNull()) {
            qDebug() << "Failed to create QImage";
        }
        else {
            auto filePath = generateIncrementedFileName("scan", "jpeg");
            img.save(filePath, "JPEG");
            pages.append(filePath);
            qDebug() << "Image scanned and saved to" << filePath;
        }

        auto wiaDeviceProps = createSubObject(device, "Properties");
        auto props = getWIAProperties(wiaDeviceProps.data());
        qDebug() << "PROPS" << props;

        if (props.contains("Document Handling Select")) {
            //check for document feeder
            if (props["Document Handling Select"].toUInt() & WIA_DPS_DOCUMENT_HANDLING_SELECT::WIA_FEEDER)
                hasMorePages = props["Document Handling Status"].toUInt() & WIA_DPS_DOCUMENT_HANDLING_STATUS::WIA_FEED_READY;
            // unfortunately code above does not work properly for ADF mode.
            // we would know if feeder become empty by catching exception code WIA_ERROR_PAPER_EMPTY
        }
    } while (hasMorePages);

    return pages;
}

void WiaDemoDialog::printObjectInfo(QAxObject* p, const QString& fileName)
{
    if (p && !p->isNull() && !fileName.isEmpty()) {
        QFile doc(fileName);
        doc.open(QFile::Truncate | QFile::ReadWrite);
        doc.write(p->generateDocumentation().toLocal8Bit());
    }
}

void WiaDemoDialog::printObjectProperties(QAxObject* obj) const
{
    if (!obj)
        return;

    auto metaobj = obj->metaObject();
    for (int i = 0; i != metaobj->propertyCount(); i++) {
        auto prop = metaobj->property(i);
        qDebug() << QString::fromLatin1(prop.name()) << ": " << prop.read(obj);
    }
}

QString WiaDemoDialog::deviceTypeToString(int type) const
{
    switch (type) {
        case StiDeviceTypeDefault: return "UnspecifiedDeviceType";
        case StiDeviceTypeScanner: return "ScannerDeviceType";
        case StiDeviceTypeDigitalCamera: return "CameraDeviceType";
        case StiDeviceTypeStreamingVideo: return "VideoDeviceType";
    }

    return QString();
}

void WiaDemoDialog::setWIAProperty(QAxObject* properties, const QString& name, const QVariant& value)
{
    if (properties->isNull())
        return;

    bool propExists = properties->dynamicCall("Exists(QVariant&)", name).toBool();
    if (!propExists)
        return;

        // Get the number of devices
    int propCount = properties->property("Count").toInt();

    // Iterate through the properties
    for (int i = 1; i <= propCount; ++i)
    {
        QAxObject* prop = properties->querySubObject("Item(QVariant&)", i);
        if (prop && prop->property("Name") == name) {
            prop->setProperty("Value", value);
            break;
            delete prop;
        }
    }
}

QVariantMap WiaDemoDialog::getWIAProperties(QAxObject* properties)
{
    QVariantMap props;
    if (properties->isNull())
        return props;

    // Get the number of devices
    int propCount = properties->property("Count").toInt();

    // Iterate through the properties
    for (int i = 1; i <= propCount; ++i)
    {
        QAxObject* prop = properties->querySubObject("Item(QVariant&)", i);
        if (prop) {
            props[prop->property("Name").toString()] = prop->property("Value");
            delete prop;
        }
    }

    return props;
}

void WiaDemoDialog::adjustScannerSettings(QAxObject* device, QAxObject* scannerItem)
{
    if (!scannerItem || !device)
        throw std::logic_error(qUtf8Printable(tr("Scanner is not selected!")));

    QScopedPointer<QAxObject> deviceProps(device->querySubObject("Properties"));
    setWIAProperty(deviceProps.data(), "Document Handling Select", ui.cbSource->currentData().toUInt());

    QScopedPointer<QAxObject> props(scannerItem->querySubObject("Properties"));
    if (!props)
        return;

    setWIAProperty(props.data(), "Current Intent", 1);
    setWIAProperty(props.data(), "Contrast", 0);
    setWIAProperty(props.data(), "Brightness", 0);
    setWIAProperty(props.data(), "Vertical Extent", 1700);
    setWIAProperty(props.data(), "Horizontal Extent", 1250);
    setWIAProperty(props.data(), "Horizontal Start Position", 0);
    setWIAProperty(props.data(), "Vertical Start Position", 0);
    setWIAProperty(props.data(), "Horizontal Resolution", 300);
    setWIAProperty(props.data(), "Vertical Resolution", 300);
    setWIAProperty(props.data(), "Bits Per Pixel", 24);

    qDebug() << "Scanner props" << getWIAProperties(props.data());
}

QSharedPointer<QAxObject> WiaDemoDialog::createSubObject(QAxObject* parentObj, const char* subObjName, const QList<QVariant>& vars) noexcept(false)
{
    m_lastErrorString.clear();
    m_lastErrorCode = 0;
    QSharedPointer<QAxObject> obj;
    if (!parentObj)
        throw std::invalid_argument(qUtf8Printable(tr("Invalid argument(s)!")));

    // This strange code below because 'querySubObject' lose arguments past by QVariantList
    if (vars.length() == 0) 
        obj.reset(parentObj->querySubObject(subObjName));
    else if (vars.length() == 1)
        obj.reset(parentObj->querySubObject(subObjName, vars[0]));
    else if (vars.length() == 2)
        obj.reset(parentObj->querySubObject(subObjName, vars[0], vars[1]));
    else if (vars.length() == 3)
        obj.reset(parentObj->querySubObject(subObjName, vars[0], vars[1], vars[2]));
    else 
        obj.reset(parentObj->querySubObject(subObjName, vars));

    if (obj) {
        connect(obj.data(), SIGNAL(exception(int, const QString&, const QString&, const QString&)), this, SLOT(slot_exception(int, const QString&, const QString&, const QString&)));
        connect(obj.data(), SIGNAL(propertyChanged(const QString&)), this, SLOT(slot_propertyChanged(const QString&)));
        connect(obj.data(), SIGNAL(signal(const QString&, int, void*)), this, SLOT(slot_signal(const QString&, int, void*)));
    }

    if (m_lastErrorString.length() > 0)
        throw std::runtime_error(qUtf8Printable(m_lastErrorString));

    if (obj.isNull()) {
        m_lastErrorString = QString("Failed create subobject") +": " + subObjName;
        throw std::runtime_error(qUtf8Printable(m_lastErrorString));
    }

    return obj;
}

void WiaDemoDialog::slot_exception(int code, const QString& source, const QString& desc, const QString& help)
{
    qDebug() << "Caught exception:";
    qDebug() << QString("Code: %1\nSource:%2\nDescription:%3\nHelp:%4").arg(code).arg(source).arg(desc).arg(help);
    m_lastErrorString = desc;
    m_lastErrorCode = code;
}

void WiaDemoDialog::slot_propertyChanged(const QString& name)
{
    qDebug() << "Property changed:";
    qDebug() << QString("Name: %1").arg(name);
}

void WiaDemoDialog::slot_signal(const QString& name, int argc, void* argv)
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    qDebug() << "Signal:";
    qDebug() << QString("Name: %1").arg(name);
}

QString WiaDemoDialog::generateIncrementedFileName(const QString& name, const QString& extension) const
{
    QString filePath;

    for (int i = 1; ; ++i) {
        filePath = name + QString::number(i) + "." + extension;
        if (!QFileInfo::exists(filePath))
            break;
    }

    return filePath;
}

void WiaDemoDialog::buildPdf(const QStringList& pages)
{
    if (pages.isEmpty()) {
        qDebug() << "No images provided.";
        return;
    }

    auto fileName = generateIncrementedFileName("Scan", "pdf");
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    printer.setPageSize(QPageSize(QPageSize::A4));

    QPainter painter;
    if (!painter.begin(&printer)) {
        qDebug() << "Failed to open file for writing: " << fileName;
        return;
    }

    for (int i = 0; i < pages.size(); ++i) {
        QImage image(pages[i]);
        if (image.isNull()) {
            qDebug() << "Failed to load image: " << pages[i];
            continue;
        }

        QRect rect = painter.viewport();
        QSize size = image.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(image.rect());

        painter.drawImage(0, 0, image);

        if (i < pages.size() - 1) {
            printer.newPage();
        }
    }
}
