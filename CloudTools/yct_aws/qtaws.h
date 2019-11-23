// Qt Interface for Amazon AWS Request Signing
//
// Based on the QtS3 library by Morten Johan Sørvig
// Licensed under The MIT License (MIT)
// Copyright (c) 2015 Morten Johan Sørvig
// https://github.com/msorvig/qts3

// TODO: Add timeout for REST requests


#ifndef QTAWS_P_H
#define QTAWS_P_H

#include "qtaws.h"
#include "qtawsqnam.h"

#include <QLoggingCategory>
#include <QtNetwork>

#include <functional>


class QtAWSReply;
class QtAWSReplyPrivate;


class QtAWSReply
{
public:
    enum AWSError
    {
        NoError,
        NetworkError,
        CredentialsError,
        InternalSignatureError,
        InternalReplyInitializationError,
        InternalError,
        UnknownError,
    };

    QtAWSReply(QtAWSReplyPrivate *replyPrivate);

    // error handling
    bool isSuccess();
    QNetworkReply::NetworkError networkError();
    QString  networkErrorString();
    AWSError awsError();
    QString  awsErrorString();
    QString  anyErrorString();

    void printReply();

    // verbatim reply as returned by AWS
    QByteArray replyData();

protected:
    QtAWSReplyPrivate *d; // ### should be explicitly shared.
};



class QtAWSReplyPrivate
{
public:
    QtAWSReplyPrivate();
    QtAWSReplyPrivate(QtAWSReply::AWSError, QString errorString);

    QByteArray                  m_byteArrayData;
    QNetworkReply*              m_networkReply;
    QtAWSReply::AWSError        m_awsError;
    QString                     m_awsErrorString;

    bool isSuccess();
    QNetworkReply::NetworkError networkError();
    QString                     networkErrorString();
    QtAWSReply::AWSError        awsError();
    QString                     awsErrorString();
    QString                     anyErrorString();

    void       prettyPrintReply();
    QByteArray headerValue(const QByteArray &headerName);
    QByteArray bytearrayValue();
};



class QtAWSPrivate
{
public:
    QtAWSPrivate();
    ~QtAWSPrivate();
    QtAWSPrivate(QByteArray accessKeyId, QByteArray secretAccessKey);
    QtAWSPrivate(std::function<QByteArray()> accessKeyIdProvider,
                 std::function<QByteArray()> secretAccessKeyProvider);

    std::function<QByteArray()> m_accessKeyIdProvider;
    std::function<QByteArray()> m_secretAccessKeyProvider;
    QByteArray m_service;
    ThreadsafeBlockingNetworkAccesManager* m_networkAccessManager;

    class AWSKeyStruct
    {
    public:
        QDateTime timeStamp;
        QByteArray key;
    };
    QHash<QByteArray, AWSKeyStruct> m_signingKeys;  // region -> key struct
    QReadWriteLock                  m_signingKeysLock;

    static QByteArray hash(const QByteArray &data);
    static QByteArray sign(const QByteArray &key, const QByteArray &data);

    static QByteArray deriveSigningKey(const QByteArray &secretAccessKey,
                                       const QByteArray dateString, const QByteArray &region,
                                       const QByteArray &m_service);

    static QByteArray formatDate(const QDate &date);
    static QByteArray formatDateTime(const QDateTime &dateTime);
    static QByteArray formatHeaderNameValueList(const QMap<QByteArray, QByteArray> &headers);
    static QByteArray formatHeaderNameList(const QMap<QByteArray, QByteArray> &headers);
    static QByteArray createCanonicalQueryString(const QByteArray &queryString);
    static QMap<QByteArray, QByteArray> canonicalHeaders(const QHash<QByteArray, QByteArray> &headers);
    static QHash<QByteArray, QByteArray> parseHeaderList(const QStringList &headers);

    static QByteArray formatCanonicalRequest(const QByteArray &method, const QByteArray &url,
                                             const QByteArray &queryString,
                                             const QHash<QByteArray, QByteArray> &headers,
                                             const QByteArray &payloadHash);

    static QByteArray formatStringToSign(const QDateTime &timeStamp, const QByteArray &m_region,
                                         const QByteArray &m_service,
                                         const QByteArray &canonicalReqeustHash);

    static QByteArray
    formatAuthorizationHeader(const QByteArray &awsAccessKeyId, const QDateTime &timeStamp,
                              const QByteArray &m_region, const QByteArray &m_service,
                              const QByteArray &signedHeaders, const QByteArray &signature);

    static QByteArray signRequestData(const QHash<QByteArray, QByteArray> headers,
                                      const QByteArray &verb, const QByteArray &url,
                                      const QByteArray &queryString, const QByteArray &payload,
                                      const QByteArray &signingKey, const QDateTime &dateTime,
                                      const QByteArray &m_region, const QByteArray &m_service);

    static QByteArray
    createAuthorizationHeader(const QHash<QByteArray, QByteArray> headers, const QByteArray &verb,
                              const QByteArray &url, const QByteArray &queryString,
                              const QByteArray &payload, const QByteArray &accessKeyId,
                              const QByteArray &signingKey, const QDateTime &dateTime,
                              const QByteArray &m_region, const QByteArray &m_service);

    // Signing key management
    static bool checkGenerateSigningKey(QHash<QByteArray, QtAWSPrivate::AWSKeyStruct> *signingKeys,
                                        const QDateTime &now,
                                        std::function<QByteArray()> secretAccessKeyProvider,
                                        const QByteArray &m_region, const QByteArray &m_service);

    // QNetworkRequest creation and signing;
    static QHash<QByteArray, QByteArray> requestHeaders(const QNetworkRequest *request);

    static void setRequestAttributes(QNetworkRequest *request, const QUrl &url,
                                     const QHash<QByteArray, QByteArray> &headers,
                                     const QDateTime &timeStamp, const QByteArray &m_host);

    static void signRequest(QNetworkRequest *request, const QByteArray &verb,
                            const QByteArray &payload, const QByteArray accessKeyId,
                            const QByteArray &signingKey, const QDateTime &dateTime,
                            const QByteArray &m_region, const QByteArray &m_service);

    // Error handling
    static QHash<QByteArray, QByteArray> getErrorComponents(const QByteArray &errorString);

    // Top-level stateful functions. These read object state and may/will modify it in a thread-safe way.
    void init();
    void checkGenerateAWSSigningKey(const QByteArray &region);

    QNetworkRequest* createSignedRequest(const QByteArray &verb, const QUrl &url,
                                         const QHash<QByteArray, QByteArray> &headers,
                                         const QByteArray &host, const QByteArray &payload,
                                         const QByteArray &region);

    QNetworkReply* sendRequest(const QByteArray &verb, const QNetworkRequest &request,
                               const QByteArray &payload);

    QtAWSReplyPrivate* sendAWSRequest(const QByteArray &verb,    const QByteArray  &host,
                                      const QByteArray &path,    const QByteArray &query,
                                      const QByteArray &region,
                                      const QByteArray &content, const QStringList &headers);

    void processNetworkReplyState(QtAWSReplyPrivate *awsReply, QNetworkReply *networkReply);

    void clearCaches();
    QByteArray accessKeyId();
    QByteArray secretAccessKey();
};



class QtAWSRequest
{
public:
    QtAWSRequest(const QString &accessKeyId, const QString &secretAccessKey);
    QtAWSRequest(std::function<QByteArray()> accessKeyIdProvider,
                 std::function<QByteArray()> secretAccessKeyProvider);

    QtAWSReply sendRequest(const QByteArray &verb,    const QByteArray  &host,
                           const QByteArray &path,    const QByteArray &query,
                           const QByteArray &region,
                           const QByteArray &content, const QStringList &headers);

    void clearCaches();
    QByteArray accessKeyId();
    QByteArray secretAccessKey();

private:
    QSharedPointer<QtAWSPrivate> d;

};


#endif
