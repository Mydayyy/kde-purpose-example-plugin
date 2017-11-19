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

const char* ENDPOINT_URL = "https://api.mydayyy.eu/imageupload";

class MydayyyShareJob : public Purpose::Job
{
    Q_OBJECT
    public:
        MydayyyShareJob(QObject* parent)
            : Purpose::Job(parent)
        {}

        enum {
            InvalidFoo = UserDefinedError,
            GenericError,
            NotConnectedError,
            UnknownMimeError
         };

        void start() override {
            // Backend does not support multiple file upload
            // Or file uploads as an album
            const QJsonValue url = data().value(QStringLiteral("urls")).toArray().first();
            QString u = url.toString();
            KIO::StoredTransferJob* job = KIO::storedGet(QUrl(u));
            connect(job, &KJob::finished, this, &MydayyyShareJob::fileFetched);
           //return QTimer::singleShot(1, this, SLOT(emitGenericError()));
        }

        void fileFetched(KJob* j) {
            if (j->error()) {
                setError(j->error());
                setErrorText(j->errorText());
                emitResult();
                return;
            }
            KIO::StoredTransferJob* job = qobject_cast<KIO::StoredTransferJob*>(j);

            QMimeDatabase db;
            QMimeType ptr = db.mimeTypeForData(job->data());
            QString mime  = ptr.name();

            if (mime.isEmpty())
            {
                return emitMimeNotFound();
            }

            QString suffix = ptr.preferredSuffix();

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
            multiPart->setParent(reply);

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

            if(reply->error() != QNetworkReply::NoError) {
                return emitGenericError();
            }

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
