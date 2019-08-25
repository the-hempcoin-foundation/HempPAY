#include "mainwindow.h"
#include "settings.h"

Settings* Settings::instance = nullptr;

Settings* Settings::init() {    
    if (instance == nullptr) 
        instance = new Settings();

    return instance;
}

Settings* Settings::getInstance() {
    return instance;
}

Config Settings::getSettings() {
    // Load from the QT Settings. 
    QSettings s;
    
    auto host        = s.value("connection/host").toString();
    auto port        = s.value("connection/port").toString();
    auto username    = s.value("connection/rpcuser").toString();
    auto password    = s.value("connection/rpcpassword").toString();    

    return Config{host, port, username, password};
}

void Settings::saveSettings(const QString& host, const QString& port, const QString& username, const QString& password) {
    QSettings s;

    s.setValue("connection/host", host);
    s.setValue("connection/port", port);
    s.setValue("connection/rpcuser", username);
    s.setValue("connection/rpcpassword", password);

    s.sync();

    // re-init to load correct settings
    init();
}

void Settings::saveRestoreTableHeader(QTableView* table, QDialog* d, QString tablename) {
    table->horizontalHeader()->restoreState(QSettings().value(tablename).toByteArray());
    table->horizontalHeader()->setStretchLastSection(true);

    QObject::connect(d, &QDialog::finished, [=](auto) {
        QSettings().setValue(tablename, table->horizontalHeader()->saveState());
    });
}

void Settings::setUsingZcashConf(QString confLocation) {
    if (!confLocation.isEmpty())
        _confLocation = confLocation;
}

bool Settings::isTestnet() {
    return _isTestnet;
}

void Settings::setTestnet(bool isTestnet) {
    this->_isTestnet = isTestnet;
}

bool Settings::isSaplingAddress(QString addr) {
    if (!isValidAddress(addr))
        return false;

    return ( isTestnet() && addr.startsWith("ztestsapling")) ||
           (!isTestnet() && addr.startsWith("zs1"));
}

bool Settings::isSproutAddress(QString addr) {
    if (!isValidAddress(addr))
        return false;
        
    return isZAddress(addr) && !isSaplingAddress(addr);
}

bool Settings::isZAddress(QString addr) {
    if (!isValidAddress(addr))
        return false;
        
    return addr.startsWith("z");
}

bool Settings::isTAddress(QString addr) {
    if (!isValidAddress(addr))
        return false;
        
    return addr.startsWith("t");
}

int Settings::getZcashdVersion() {
    return _zcashdVersion;
}

void Settings::setZcashdVersion(int version) {
    _zcashdVersion = version;
}

bool Settings::isSyncing() {
    return _isSyncing;
}

void Settings::setSyncing(bool syncing) {
    this->_isSyncing = syncing;
}

int Settings::getBlockNumber() {
    return this->_blockNumber;
}

void Settings::setBlockNumber(int number) {
    this->_blockNumber = number;
}

bool Settings::isSaplingActive() {
    return  (isTestnet() && getBlockNumber() > 0) || (!isTestnet() && getBlockNumber() > 0);
}

double Settings::getZECPrice() { 
    return zecPrice; 
}

bool Settings::getAutoShield() {
    // Load from Qt settings
    return QSettings().value("options/autoshield", false).toBool();
}

void Settings::setAutoShield(bool allow) {
    QSettings().setValue("options/autoshield", allow);
}

bool Settings::getAllowCustomFees() {
    // Load from the QT Settings. 
    return QSettings().value("options/customfees", false).toBool();
}

void Settings::setAllowCustomFees(bool allow) {
    QSettings().setValue("options/customfees", allow);
}

bool Settings::getSaveZtxs() {
    // Load from the QT Settings. 
    return QSettings().value("options/savesenttx", true).toBool();
}

void Settings::setSaveZtxs(bool save) {
    QSettings().setValue("options/savesenttx", save);
}

void Settings::setPeers(int peers) {
    _peerConnections = peers;
}

int Settings::getPeers() {
    return _peerConnections;
}
//=================================
// Static Stuff
//=================================
void Settings::saveRestore(QDialog* d) {
    d->restoreGeometry(QSettings().value(d->objectName() % "geometry").toByteArray());

    QObject::connect(d, &QDialog::finished, [=](auto) {
        QSettings().setValue(d->objectName() % "geometry", d->saveGeometry());
    });
}

QString Settings::getUSDFormat(double bal) {
    return "$" + QLocale(QLocale::English).toString(bal * Settings::getInstance()->getZECPrice(), 'f', 2);
}

QString Settings::getDecimalString(double amt) {
    QString f = QString::number(amt, 'f', 8);

    while (f.contains(".") && (f.right(1) == "0" || f.right(1) == ".")) {
        f = f.left(f.length() - 1);
    }
    if (f == "-0")
        f = "0";

    return f;
}

QString Settings::getZECDisplayFormat(double bal) {
    // This is idiotic. Why doesn't QString have a way to do this?
    return getDecimalString(bal) % " " % Settings::getTokenName();
}

QString Settings::getZECUSDDisplayFormat(double bal) {
    auto usdFormat = getUSDFormat(bal);
    if (!usdFormat.isEmpty())
        return getZECDisplayFormat(bal) % " (" % getUSDFormat(bal) % ")";
    else
        return getZECDisplayFormat(bal);
}

const QString Settings::txidStatusMessage = QString(QObject::tr("Tx submitted (right click to copy) txid:"));

QString Settings::getTokenName() {
    if (Settings::getInstance()->isTestnet()) {
        return "THCT";
    } else {
        return "THC";
    }
}

QString Settings::getDonationAddr(bool sapling) {
    if (Settings::getInstance()->isTestnet())  {
	    return "ztestsaplingXXX";
    }
    return "zs1n7auhuf06gzzj6gl3f3gvelnr9ez27eafmtyj9s649fat77zg8l8mz8f937kd2nacq23veu2fc6";
}

bool Settings::addToZcashConf(QString confLocation, QString line) {
    QFile file(confLocation);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Append))
        return false;
    

    QTextStream out(&file);
    out << line << "\n";
    file.close();

    return true;
}

bool Settings::removeFromZcashConf(QString confLocation, QString option) {
    if (confLocation.isEmpty())
        return false;

    // To remove an option, we'll create a new file, and copy over everything but the option.
    QFile file(confLocation);
    if (!file.open(QIODevice::ReadOnly)) 
        return false;
    
    QList<QString> lines;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        auto s = line.indexOf("=");
        QString name = line.left(s).trimmed().toLower();
        if (name != option) {
            lines.append(line);
        }
    }    
    file.close();
    
    QFile newfile(confLocation);
    if (!newfile.open(QIODevice::ReadWrite | QIODevice::Truncate))
        return false;

    QTextStream out(&newfile);
    for (QString line : lines) {
        out << line << endl;
    }
    newfile.close();

    return true;
}

double Settings::getMinerFee() {
    return 0.0001;
}

double Settings::getZboardAmount() {
    return 0.0001;
}

QString Settings::getZboardAddr() {
    if (Settings::getInstance()->isTestnet()) {
        return getDonationAddr(true);
    }
    else {
        return "zs10m00rvkhfm4f7n23e4sxsx275r7ptnggx39ygl0vy46j9mdll5c97gl6dxgpk0njuptg2mn9w5s";
    }
}

bool Settings::isValidSaplingPrivateKey(QString pk) {
    if (isTestnet()) {
        QRegExp zspkey("^secret-extended-key-test[0-9a-z]{278}$", Qt::CaseInsensitive);
        return zspkey.exactMatch(pk);
    } else {
        QRegExp zspkey("^secret-extended-key-main[0-9a-z]{278}$", Qt::CaseInsensitive);
        return zspkey.exactMatch(pk);
    }
}

bool Settings::isValidAddress(QString addr) {
    QRegExp zsexp("^zs1[a-z0-9]{75}$",  Qt::CaseInsensitive);
    QRegExp ztsexp("^ztestsapling[a-z0-9]{76}", Qt::CaseInsensitive);
    QRegExp texp("^R[a-z0-9]{33}$", Qt::CaseInsensitive);
    //qDebug() << "isValidAddress(" << addr << ")";

    return  texp.exactMatch(addr) || ztsexp.exactMatch(addr) || zsexp.exactMatch(addr);
}

// Get a pretty string representation of this Payment URI
QString Settings::paymentURIPretty(PaymentURI uri) {
    return QString() + "Payment Request\n" + "Pay: " + uri.addr + "\nAmount: " + getZECDisplayFormat(uri.amt.toDouble()) 
        + "\nMemo:" + QUrl::fromPercentEncoding(uri.memo.toUtf8());
}

// Parse a payment URI string into its components
PaymentURI Settings::parseURI(QString uri) {
    PaymentURI ans;

    if (!uri.startsWith("thc:")) {
        ans.error = "Not a THC payment URI";
        return ans;
    }

    uri = uri.right(uri.length() - QString("thc:").length());
    
    QRegExp re("([a-zA-Z0-9]+)");
    int pos;
    if ( (pos = re.indexIn(uri)) == -1 ) {
        ans.error = "Couldn't find an address";
        return ans;
    }

    ans.addr = re.cap(1);
    if (!Settings::isValidAddress(ans.addr)) {
        ans.error = "Could not understand address";
        return ans;
    }
    uri = uri.right(uri.length() - ans.addr.length());

    if (!uri.isEmpty()) {
        uri = uri.right(uri.length() - 1); // Eat the "?"

        QStringList args = uri.split("&");
        for (QString arg: args) {
            QStringList kv = arg.split("=");
            if (kv.length() != 2) {
                ans.error = "No value argument was seen";
                return ans;
            }

            if (kv[0].toLower() == "amt" || kv[0].toLower() == "amount") {
                ans.amt = kv[1];
            } else if (kv[0].toLower() == "memo" || kv[0].toLower() == "message" || kv[0].toLower() == "msg") {
                ans.memo = QUrl::fromPercentEncoding(kv[1].toUtf8());
            } else {
                // Ignore unknown fields, since some developers use it to pass extra data.
            }
        }
    }

    return ans;
}

const QString Settings::labelRegExp("[a-zA-Z0-9\\-_]{0,40}");
