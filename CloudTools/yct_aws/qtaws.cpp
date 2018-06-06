#include <QtCore>

#include "qtaws.h"


//  Data flow for requests:
//
//                        AWS Secret key ->
//                                Date   -> Signing
//                               Region  ->   Key    ---------------|
//                               Service ->                         |
//                                                                  |
//        Headers: Host, X-Amz-Date                                 |  DateTime
//  App                 |                                           |  Date/region/service
//   |                  |                                           |    |
//    Request  -> Canonical Request -> CanonicalRequest Hash -> String to Sign -> Request Signature
//        |                                                             |             |
//    Request                                                           |             |
//   with Auth  <---------------Athentication Header-----------------------------------
//     header                             ^
//        |                          AWS Key Id
//    THE INTERNET
//
//  Control flow for request signing:
//
//  checkGenerateSigningKey
//      deriveSigningKey
//
//  createAuthorizationHeader
//      signRequestData
//          formatCanonicalRequest
//          formatStringToSign
//      formatAuthorizationHeader


QtAWSRequest::QtAWSRequest(const QString &accessKeyId, const QString &secretAccessKey,
                           const QString &service)
    : d(new QtAWSPrivate(accessKeyId.toLatin1(),
                         secretAccessKey.toLatin1(),
                         service.toLatin1()))
{
}


QtAWSRequest::QtAWSRequest(std::function<QByteArray()> accessKeyIdProvider,
           std::function<QByteArray()> secretAccessKeyProvider)
    : d(new QtAWSPrivate(accessKeyIdProvider, secretAccessKeyProvider))
{
}


void QtAWSRequest::clearCaches()
{
    d->clearCaches();
}


QByteArray QtAWSRequest::accessKeyId()
{
    return d->accessKeyId();
}


QByteArray QtAWSRequest::secretAccessKey()
{
    return d->secretAccessKey();
}


QtAWSReply QtAWSRequest::sendRequest(const QByteArray &verb,    const QByteArray  &host,
                                      const QByteArray &path,    const QByteArray &query,
                                      const QByteArray &region,
                                      const QByteArray &content, const QStringList &headers)
{
    return QtAWSReply(d->sendAWSRequest(verb,host,path,query,region,content,headers));
}



QtAWSPrivate::QtAWSPrivate()
{
    m_networkAccessManager=0;
    m_service="";
}


QtAWSPrivate::QtAWSPrivate(QByteArray accessKeyId, QByteArray secretAccessKey, QByteArray service)
{
    m_accessKeyIdProvider = [accessKeyId](){ return accessKeyId; };
    m_secretAccessKeyProvider = [secretAccessKey](){ return secretAccessKey; };
    m_service=service;

    init();
}


QtAWSPrivate::QtAWSPrivate(std::function<QByteArray()> accessKeyIdProvider,
                           std::function<QByteArray()> secretAccessKeyProvider)
    : m_accessKeyIdProvider(accessKeyIdProvider),
      m_secretAccessKeyProvider(secretAccessKeyProvider)
{
    m_service="";

    init();
}


QtAWSPrivate::~QtAWSPrivate()
{
    if (m_networkAccessManager)
    {
        if (m_networkAccessManager->pendingRequests() > 0)
        {
            qWarning() << "QtAWS object deleted with pending requests";
        }

        delete m_networkAccessManager;
        m_networkAccessManager=0;
    }
}


// Returns a date formatted as YYYYMMDD.
QByteArray QtAWSPrivate::formatDate(const QDate &date)
{
    return date.toString(QStringLiteral("yyyyMMdd")).toLatin1();
}


// Returns a date formatted as YYYYMMDDTHHMMSSZ
QByteArray QtAWSPrivate::formatDateTime(const QDateTime &dateTime)
{
    return dateTime.toString(QStringLiteral("yyyyMMddThhmmssZ")).toLatin1();
}


// SHA256.
QByteArray QtAWSPrivate::hash(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}


// HMAC_SHA256.
QByteArray QtAWSPrivate::sign(const QByteArray &key, const QByteArray &data)
{
    return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
}


// Canonicalizes a list of http headers.
//    * sorted on header key (using QMap)
//    * whitespace trimmed.
QMap<QByteArray, QByteArray> QtAWSPrivate::canonicalHeaders(
    const QHash<QByteArray, QByteArray> &headers)
{
    QMap<QByteArray, QByteArray> canonical;

    for (auto it = headers.begin(); it != headers.end(); ++it)
        canonical[it.key().toLower()] = it.value().trimmed();
    return canonical;
}


// Creates newline-separated list of headers formatted like "name:values"
QByteArray QtAWSPrivate::formatHeaderNameValueList(const QMap<QByteArray, QByteArray> &headers)
{
    QByteArray nameValues;
    for (auto it = headers.begin(); it != headers.end(); ++it)
        nameValues += it.key() + ":" + it.value() + "\n";
    return nameValues;
}


// Creates a semicolon-separated list of header names
QByteArray QtAWSPrivate::formatHeaderNameList(const QMap<QByteArray, QByteArray> &headers)
{
    QByteArray names;
    for (auto it = headers.begin(); it != headers.end(); ++it)
        names += it.key() + ";";
    names.chop(1); // remove final ";"
    return names;
}


// Creates a canonical query string by sorting and percent encoding the query
QByteArray QtAWSPrivate::createCanonicalQueryString(const QByteArray &queryString)
{
    // General querystring form
    // "Foo1=bar1&"
    // "Foo2=bar2&"

    // remove any leading '?'
    QByteArray input = queryString;
    //    if (input.startsWith("?"))
    //        input.remove(0, 1);

    // split and sort querty parts
    QList<QByteArray> parts = input.split('&');
    qSort(parts);
    // write out percent encoded canonical string.
    QByteArray canonicalQuery;
    canonicalQuery.reserve(queryString.size());
    //    canonicalQuery.append("%3F");
    foreach (const QByteArray &part, parts)
    {
        QByteArray encodedPart = part.toPercentEncoding("=%"); // skip = and % for aws complicance
        if (!encodedPart.isEmpty() && !encodedPart.contains('='))
            encodedPart.append("=");

        canonicalQuery.append(encodedPart);
        canonicalQuery.append('&');
    }
    canonicalQuery.chop(1); // remove final '&'

    return canonicalQuery;
}


//  Derives an AWS version 4 signing key. \a secretAccessKey is the aws secrect key,
//  \a dateString is a YYYYMMDD date. The signing key is valid for a limited number of days
//  (currently 7). \a region is the bucket region, for example "us-east-1". \a service the
//  aws service ("s3", ...)
QByteArray QtAWSPrivate::deriveSigningKey(const QByteArray &secretAccessKey,
                                          const QByteArray dateString, const QByteArray &region,
                                          const QByteArray &service)
{
    return sign(sign(sign(sign("AWS4" + secretAccessKey, dateString), region), service),
                "aws4_request");
}


// Generates a new AWS signing key when required. This will typically happen
// on the first call or when the key expires.
bool QtAWSPrivate::checkGenerateSigningKey(QHash<QByteArray, QtAWSPrivate::AWSKeyStruct> *signingKeys,
                                           const QDateTime &now,
                                           std::function<QByteArray()> secretAccessKeyProvider,
                                           const QByteArray &region, const QByteArray &service)
{
    const int secondsInDay = 60 * 60 * 24;

    auto it = signingKeys->find(region);
    if (it != signingKeys->end() && it->timeStamp.isValid())
    {
        const qint64 keyAge = it->timeStamp.secsTo(now);
        if (keyAge >= 0 && keyAge < secondsInDay)
            return false;
    }

    QByteArray key =
        deriveSigningKey(secretAccessKeyProvider(), formatDate(now.date()), region, service);
    AWSKeyStruct keyStruct = {now, key};
    signingKeys->insert(region, keyStruct);

    return true;
}


// Creates a "stringToSign" (see aws documentation).
QByteArray QtAWSPrivate::formatStringToSign(const QDateTime &timeStamp, const QByteArray &region,
                                            const QByteArray &service,
                                            const QByteArray &canonicalRequestHash)
{
    QByteArray string = "AWS4-HMAC-SHA256\n" + formatDateTime(timeStamp) + "\n"
                        + formatDate(timeStamp.date()) + "/" + region + "/" + service
                        + "/aws4_request\n" + canonicalRequestHash;
    return string;
}


// Formats an authorization header.
QByteArray QtAWSPrivate::formatAuthorizationHeader(
    const QByteArray &awsAccessKeyId, const QDateTime &timeStamp, const QByteArray &region,
    const QByteArray &service, const QByteArray &signedHeaders, const QByteArray &signature)
{
    QByteArray headerValue = "AWS4-HMAC-SHA256 "
                             "Credential=" + awsAccessKeyId + "/" + formatDate(timeStamp.date())
                             + "/" + region + "/" + service + "/aws4_request, " + +"SignedHeaders="
                             + signedHeaders + ", " + "Signature=" + signature;
    return headerValue;
}


// Copies the request headers from a QNetworkRequest.
QHash<QByteArray, QByteArray> QtAWSPrivate::requestHeaders(const QNetworkRequest *request)
{
    QHash<QByteArray, QByteArray> headers;
    for (const QByteArray &header : request->rawHeaderList())
    {
        headers.insert(header, request->rawHeader(header));
    }
    return headers;
}


QHash<QByteArray, QByteArray> QtAWSPrivate::parseHeaderList(const QStringList &headers)
{
    QHash<QByteArray, QByteArray> parsedHeaders;
    foreach (const QString &header, headers)
    {
        QStringList parts = header.split(":");
        parsedHeaders.insert(parts.at(0).toLatin1(), parts.at(1).toLatin1());
    }
    return parsedHeaders;
}


// Creates a canonical request string (example):
//     POST
//     /
//
//     content-type:application/x-www-form-urlencoded; charset=utf-8\n
//     host:iam.amazonaws.com\n
//     x-amz-date:20110909T233600Z\n
//
//     content-type;host;x-amz-date\n
//     b6359072c78d70ebee1e81adcbab4f01bf2c23245fa365ef83fe8f1f955085e2
QByteArray QtAWSPrivate::formatCanonicalRequest(const QByteArray &method, const QByteArray &url,
                                                const QByteArray &queryString,
                                                const QHash<QByteArray, QByteArray> &headers,
                                                const QByteArray &payloadHash)
{
    const auto canonHeaders = canonicalHeaders(headers);
    const int estimatedLength = method.length() + url.length() + queryString.length()
                                + canonHeaders.size() * 10 + payloadHash.length();

    QByteArray request;
    request.reserve(estimatedLength);
    request += method;
    request += "\n";
    request += url;
    request += "\n";
    request += createCanonicalQueryString(queryString);
    request += "\n";
    request += formatHeaderNameValueList(canonHeaders);
    request += "\n";
    request += formatHeaderNameList(canonHeaders);
    request += "\n";
    request += payloadHash;
    return request;
}


QByteArray QtAWSPrivate::signRequestData(const QHash<QByteArray, QByteArray> headers,
                                         const QByteArray &verb, const QByteArray &url,
                                         const QByteArray &queryString, const QByteArray &payload,
                                         const QByteArray &signingKey, const QDateTime &dateTime,
                                         const QByteArray &region, const QByteArray &service)
{
    // create canonical request representation and hash
    QByteArray payloadHash = hash(payload).toHex();
    QByteArray canonicalRequest =
        formatCanonicalRequest(verb, url, queryString, headers, payloadHash);
    QByteArray canonialRequestHash = hash(canonicalRequest).toHex();

    // create (and sign) stringToSign
    QByteArray stringToSign = formatStringToSign(dateTime, region, service, canonialRequestHash);
    QByteArray signature    = sign(signingKey, stringToSign);

    return signature;
}


QByteArray QtAWSPrivate::createAuthorizationHeader(
    const QHash<QByteArray, QByteArray> headers, const QByteArray &verb, const QByteArray &url,
    const QByteArray &queryString, const QByteArray &payload, const QByteArray &accessKeyId,
    const QByteArray &signingKey, const QDateTime &dateTime, const QByteArray &region,
    const QByteArray &service)
{
    // sign request
    QByteArray signature = signRequestData(headers, verb, url, queryString, payload, signingKey,
                                           dateTime, region, service);

    // crate Authorization header;
    QByteArray headerNames = formatHeaderNameList(canonicalHeaders(headers));
    return formatAuthorizationHeader(accessKeyId, dateTime, region, service, headerNames,
                                     signature.toHex());
}


void QtAWSPrivate::setRequestAttributes(QNetworkRequest *request, const QUrl &url,
                                        const QHash<QByteArray, QByteArray> &headers,
                                        const QDateTime &timeStamp, const QByteArray &host)
{
    // Build request from user input
    request->setUrl(url);
    for (auto it = headers.begin(); it != headers.end(); ++it)
        request->setRawHeader(it.key(), it.value());

    // Add standard AWS headers
    request->setRawHeader("User-Agent", "Qt");
    request->setRawHeader("Host", host);
    request->setRawHeader("X-Amz-Date", formatDateTime(timeStamp));
}


// Signs an aws request by adding an authorization header
void QtAWSPrivate::signRequest(QNetworkRequest *request, const QByteArray &verb,
                               const QByteArray &payload, const QByteArray accessKeyId,
                               const QByteArray &signingKey, const QDateTime &dateTime,
                               const QByteArray &region, const QByteArray &service)
{
    QByteArray payloadHash = hash(payload).toHex();
    request->setRawHeader("x-amz-content-sha256", payloadHash);

    // get headers from request
    QHash<QByteArray, QByteArray> headers = requestHeaders(request);
    QUrl url = request->url();
    // create authorization header (value)
    QByteArray authHeaderValue
        = createAuthorizationHeader(headers, verb, url.path().toLatin1(), url.query().toLatin1(),
                                    payload, accessKeyId, signingKey, dateTime, region, service);
    // add authorization header to request
    request->setRawHeader("Authorization", authHeaderValue);
}


//
// Stateful non-static functons below
//

void QtAWSPrivate::init()
{
    // The current design multiplexes requests from several QtAWS request threads
    // to one QNetworkAccessManager on a network thread. This limits the number
    // of concurrent network requests to the QNetworkAccessManager internal limit
    // (rumored to be 6).
    m_networkAccessManager = new ThreadsafeBlockingNetworkAccesManager();

    if (m_accessKeyIdProvider().isEmpty())
    {
        qWarning() << "access key id not specified";
    }

    if (m_secretAccessKeyProvider().isEmpty())
    {
        qWarning() << "secret access key not set";
    }

    if (m_service.isEmpty())
    {
        m_service="execute-api";
    }
}


void QtAWSPrivate::checkGenerateAWSSigningKey(const QByteArray &region)
{
    QDateTime now = QDateTime::currentDateTimeUtc();
    m_signingKeysLock.lockForWrite();
    checkGenerateSigningKey(&m_signingKeys, now, m_secretAccessKeyProvider, region, m_service);
    m_signingKeysLock.unlock();
}


QNetworkRequest *QtAWSPrivate::createSignedRequest(const QByteArray &verb, const QUrl &url,
                                                   const QHash<QByteArray, QByteArray> &headers,
                                                   const QByteArray &host, const QByteArray &payload,
                                                   const QByteArray &region)
{
    checkGenerateAWSSigningKey(region);
    QDateTime requestTime = QDateTime::currentDateTimeUtc();

    // Create and sign request
    QNetworkRequest *request = new QNetworkRequest();

    // request->setAttribute(QNetworkRequest::SynchronousRequestAttribute, true);
    // request->setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

    setRequestAttributes(request, url, headers, requestTime, host);
    m_signingKeysLock.lockForRead();
    QByteArray key = m_signingKeys.value(region).key;
    m_signingKeysLock.unlock();
    signRequest(request, verb, payload, m_accessKeyIdProvider(), key, requestTime, region, m_service);
    return request;
}


QNetworkReply *QtAWSPrivate::sendRequest(const QByteArray &verb, const QNetworkRequest &request,
                                         const QByteArray &payload)
{
    QBuffer payloadBuffer(const_cast<QByteArray *>(&payload));
    if (!payload.isEmpty())
    {
        payloadBuffer.open(QIODevice::ReadOnly);
    }

    // Send request
    QNetworkReply *reply = m_networkAccessManager->sendCustomRequest(
        request, verb, payload.isEmpty() ? nullptr : &payloadBuffer);

    return reply;
}


QHash<QByteArray, QByteArray> QtAWSPrivate::getErrorComponents(const QByteArray &errorString)
{
    QHash<QByteArray, QByteArray> hash;
    QByteArray currentElement;

    QXmlStreamReader xml(errorString);
    while (!xml.atEnd())
    {
        xml.readNext();

        if (xml.isStartElement())
        {
            currentElement = xml.name().toUtf8();
            hash[currentElement] = "";
        }
        if (xml.isCharacters())
        {
            hash[currentElement] = xml.text().toUtf8();
        }
    }

    if (xml.hasError())
    {
        qDebug() << "xml err";
    }

    return hash;
}


void QtAWSPrivate::processNetworkReplyState(QtAWSReplyPrivate *awsReply, QNetworkReply *networkReply)
{
    awsReply->m_networkReply=networkReply;

    // Read the reply content
    awsReply->m_byteArrayData = networkReply->readAll();

    // No error
    if (networkReply->error()==QNetworkReply::NoError)
    {
        awsReply->m_awsError      =QtAWSReply::NoError;
        awsReply->m_awsErrorString=QString();
        return;
    }

    // By default, and if the error XML processing below fails,
    // set the S3 error to NetworkError and forward the error string.
    awsReply->m_awsError      =QtAWSReply::NetworkError;
    awsReply->m_awsErrorString=networkReply->errorString();


    // TODO: Implement API-specific error handling

    /*
    // errors: http://docs.aws.amazon.com/AmazonS3/latest/API/ErrorResponses.html
    QHash<QByteArray, QByteArray> components = getErrorComponents(awsReply->m_byteArrayData);
    awsReply->m_awsErrorString.clear();
    if (components.contains("Error"))
    {
        // TODO
        awsReply->m_awsErrorString.append(components.value("Message"));
    }
    */
}


QtAWSReplyPrivate* QtAWSPrivate::sendAWSRequest(const QByteArray &verb,    const QByteArray  &host,
                                                const QByteArray &path,    const QByteArray &query,
                                                const QByteArray &region,
                                                const QByteArray &content, const QStringList &headers)
{
    QtAWSReplyPrivate *awsReply=new QtAWSReplyPrivate;

    const QByteArray url="https://" + host + "/" + path + (query.isEmpty() ? "" : "?" + query);
    QHash<QByteArray, QByteArray> hashHeaders=parseHeaderList(headers);

    QNetworkRequest* request     =createSignedRequest(verb, QUrl(url), hashHeaders, host, content, region);
    QNetworkReply*   networkReply=sendRequest(verb, *request, content);

    processNetworkReplyState(awsReply, networkReply);

    return awsReply;
}


void QtAWSPrivate::clearCaches()
{
    m_signingKeysLock.lockForWrite();
    m_signingKeys.clear();
    m_signingKeysLock.unlock();
}


QByteArray QtAWSPrivate::accessKeyId()
{
    return m_accessKeyIdProvider();
}


QByteArray QtAWSPrivate::secretAccessKey()
{
    return m_secretAccessKeyProvider();
}


QtAWSReplyPrivate::QtAWSReplyPrivate()
    : m_networkReply(0), m_awsError(QtAWSReply::InternalReplyInitializationError),
      m_awsErrorString("Internal error: un-initianlized QtAWSReply.")
{
}


QtAWSReplyPrivate::QtAWSReplyPrivate(QtAWSReply::AWSError error, QString errorString)
    : m_networkReply(0), m_awsError(error), m_awsErrorString(errorString)
{
}


QNetworkReply::NetworkError QtAWSReplyPrivate::networkError()
{
    return m_networkReply ? m_networkReply->error() : QNetworkReply::NoError;
}


QString QtAWSReplyPrivate::networkErrorString()
{
    return m_networkReply ? m_networkReply->errorString() : QString();
}


QtAWSReply::AWSError QtAWSReplyPrivate::awsError()
{
    return m_awsError;
}


QString QtAWSReplyPrivate::awsErrorString()
{
    return m_awsErrorString;
}


QString QtAWSReplyPrivate::anyErrorString()
{
    if (networkError() != QNetworkReply::NoError)
        return networkErrorString();

    if (awsError() != QtAWSReply::NoError)
        return awsErrorString();

    return QString();
}


void QtAWSReplyPrivate::prettyPrintReply()
{
    qDebug() << "Reply:                   :" << this;
    qDebug() << "Reply Error State        :" << awsError() << awsErrorString();

    if (m_networkReply == 0)
    {
        qDebug() << "m_networkReply is null";
        return;
    }

    qDebug() << "NetworkReply Error State :" << m_networkReply->error()
             << m_networkReply->errorString();
    qDebug() << "NetworkReply Headers:";
    foreach (auto pair, m_networkReply->rawHeaderPairs())
    {
        qDebug() << "   " << pair.first << pair.second;
    }
    qDebug() << "AWS reply data            :" << bytearrayValue();
}


QByteArray QtAWSReplyPrivate::headerValue(const QByteArray &headerName)
{
    if (!m_networkReply)
    {
        return QByteArray();
    }

    return m_networkReply->rawHeader(headerName);
}


bool QtAWSReplyPrivate::isSuccess()
{
    return m_awsError == QtAWSReply::NoError;
}


QByteArray QtAWSReplyPrivate::bytearrayValue()
{
    return m_byteArrayData;
}


QtAWSReply::QtAWSReply(QtAWSReplyPrivate* replyPrivate)
    : d(replyPrivate)
{
}


bool QtAWSReply::isSuccess()
{
    return d->isSuccess();
}


QNetworkReply::NetworkError QtAWSReply::networkError()
{
    return d->networkError();
}


QString QtAWSReply::networkErrorString()
{
    return d->networkErrorString();
}


QtAWSReply::AWSError QtAWSReply::awsError()
{
    return d->awsError();
}


QString QtAWSReply::awsErrorString()
{
    return d->awsErrorString();
}

QString QtAWSReply::anyErrorString()
{
    return d->anyErrorString();
}

QByteArray QtAWSReply::replyData()
{
    return d->bytearrayValue();
}


void QtAWSReply::printReply()
{
    d->prettyPrintReply();
}
