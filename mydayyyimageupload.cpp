#include <purpose/job.h>
#include <purpose/pluginbase.h>

#include <KJob>
#include <KIO/TransferJob>
#include <KIO/StoredTransferJob>

#include <QHttpMultiPart>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QMimeDatabase>
#include <QMimeType>

#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonArray>
#include <KPluginFactory>

const char* ENDPOINT_URL = "YOUR_ENDPOINT_HERE";

class MydayyyShareJob : public Purpose::Job
{
    Q_OBJECT
    public:
        MydayyyShareJob(QObject* parent)
            : Purpose::Job(parent)
        {}

        // All errors which can be thrown
        enum {
            InvalidFoo = UserDefinedError,
            GenericError,
            NotConnectedError,
            UnknownMimeError
         };

        // Entrypoint for the share job. This will be called by purpose
        void start() override {
            const QJsonValue url = data().value(QStringLiteral("urls")).toArray().first();
            QString u = url.toString();

            // As this is an image plugin, we are receiving a data URL containing an
            // image in base64. We basically fire a http get onto this url to get the
            // raw image
            KIO::StoredTransferJob* job = KIO::storedGet(QUrl(u));
            connect(job, &KJob::finished, this, &MydayyyShareJob::fileFetched);
        }

        void fileFetched(KJob* j) {
            // If any errors were thrown, return them.
            if (j->error()) {
                setError(j->error());
                setErrorText(j->errorText());
                emitResult();
                return;
            }

            // Otherwise we check whether a mimetype was found. If we can't
            // find one, the remote server probably can't either
            KIO::StoredTransferJob* job = qobject_cast<KIO::StoredTransferJob*>(j);

            QMimeDatabase db;
            QMimeType ptr = db.mimeTypeForData(job->data());
            QString mime  = ptr.name();

            if (mime.isEmpty())
            {
                return emitMimeNotFound();
            }

            QString suffix = ptr.preferredSuffix(); // Get the suffix corresponding to the mime

            // Create a multipart http request containing the image
            QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

            QHttpPart imagePart;
            imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"image\"; filename=\"image."+suffix+"\""));
            imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant(mime));
            imagePart.setBody(job->data());

            multiPart->append(imagePart);

            QUrl url(ENDPOINT_URL);
            QNetworkRequest request(url);

            QNetworkAccessManager *networkManager= new QNetworkAccessManager;
            QNetworkReply *reply = networkManager->post(request, multiPart);
            multiPart->setParent(reply); // Delete the multipart object once reply is deleted

            connect(reply, SIGNAL(finished()), this, SLOT(uploadDone()));
        }



public slots:
        void emitGenericError() {
            setError(GenericError);
            setErrorText(QString::fromUtf8("An unknown error occured."));
            emitResult();
        }
        void emitNotConnectedError() {
            setError(NotConnectedError);
            setErrorText(QString::fromUtf8("You are not connected to the designated teamspeak server"));
            emitResult();
        }
        void emitMimeNotFound() {
            setError(UnknownMimeError);
            setErrorText(QString::fromUtf8("Could not determine mimetype"));
            emitResult();
        }
        void emitError(QString error) {
            setError(GenericError);
            setErrorText(error);
            emitResult();
        }
        void emitURL(QString url) {
            setOutput({ { QStringLiteral("url"), url } });
            emitResult();
        }
        void uploadDone() {
            QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
            reply->deleteLater();

            // Check for any http errors
            if(reply->error() != QNetworkReply::NoError) {
                if(reply->error() == QNetworkReply::ContentAccessDenied || reply->error() == QNetworkReply::RemoteHostClosedError) {
                    return emitNotConnectedError();
                }
                return emitGenericError();
            }

            // We expect a json from our server, so try to parse the response
            // and throw an error when it't not a valid json
            QByteArray ba = reply->readAll();

            QJsonParseError error;
            QJsonDocument doc = QJsonDocument::fromJson(ba, &error);

            if (error.error == QJsonParseError::NoError) {
                QJsonObject root = doc.object();
                if (root.value("success").toBool() == true) {
                    emitURL(root.value("message").toString());
                } else {
                    emitError(root.value("message").toString());
                }
            } else {
                return emitGenericError();
            }
        }
};

class Q_DECL_EXPORT MydayyyImagePlugin : public Purpose::PluginBase
{
    Q_OBJECT
    public:
        MydayyyImagePlugin(QObject* p, const QVariantList& ) : Purpose::PluginBase(p) {}

        Purpose::Job* createJob() const override
        {
            return new MydayyyShareJob(nullptr);
        }
};

K_PLUGIN_FACTORY_WITH_JSON(MydayyyImageShare, "mydayyyimageupload.json", registerPlugin<MydayyyImagePlugin>();)

#include "mydayyyimageupload.moc"
