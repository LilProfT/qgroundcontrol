/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include <QApplication>
#include <QDebug>
#include <QRegularExpression>
#include <QFile>
#include <QJsonDocument>

#include "AudioOutput.h"
#include "QGCApplication.h"
#include "QGC.h"
#include "SettingsManager.h"

AudioOutput::AudioOutput(QGCApplication* app, QGCToolbox* toolbox)
    : QGCTool   (app, toolbox)
    , _effect   (nullptr)
    , _tts      (nullptr)
{
    if (qgcApp()->runningUnitTests()) {
        // Cloud based unit tests don't have speech capabilty. If you try to crank up
        // speech engine it will pop a qWarning which prevents usage of QT_FATAL_WARNINGS
        return;
    }

    // QTextToSpeech
    _tts = new QTextToSpeech(this);

    //-- Force TTS engine to English as all incoming messages from the autopilot
    //   are in English and not localized.
#ifdef Q_OS_LINUX
    _tts->setLocale(QLocale("en_US"));
#endif
    connect(_tts, &QTextToSpeech::stateChanged, this, &AudioOutput::_stateChanged);

    // QSoundEffect
    QFile file(":/audio/audioMap.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) qDebug() << "file cannot be opened";
    _audioMap = QJsonDocument::fromJson(file.readAll()).object().toVariantMap();

    _effect = new QSoundEffect(this);
    _effect->setLoopCount(0);
    _effect->setMuted(false);
    _effect->setVolume(100);

    connect(_effect, &QSoundEffect::playingChanged, this, &AudioOutput::_effectPlayingChanged);
    connect(_effect, &QSoundEffect::statusChanged, this, &AudioOutput::_effectStateChanged);

}

void AudioOutput::say(const QString& inText)
{
    if (!_tts) {
        qDebug() << "say" << inText;
        return;
    }

    bool muted = qgcApp()->toolbox()->settingsManager()->appSettings()->audioMuted()->rawValue().toBool();
    muted |= qgcApp()->runningUnitTests();
    if (!muted && !qgcApp()->runningUnitTests()) {
        QString text = fixTextMessageForAudio(inText);
        if(_tts->state() == QTextToSpeech::Speaking) {
            if(!_texts.contains(text)) {
                //-- Some arbitrary limit
                if(_texts.size() > 20) {
                    _texts.removeFirst();
                }
                _texts.append(text);
            }
        } else if (_effect->isPlaying() || _effect->status() == QSoundEffect::Loading) {
            if(!_texts.contains(text)) {
                //-- Some arbitrary limit
                if(_texts.size() > 20) {
                    _texts.removeFirst();
                }
                qDebug() << "append ::: " << text;

                _texts.append(text);
            }
        } else {
            _play(text);
        }
    }
}

void AudioOutput::_stateChanged(QTextToSpeech::State state)
{
    if (state == QTextToSpeech::Ready) {
        _next();
    }
}

void AudioOutput::_effectPlayingChanged()
{
    qDebug() << "_effectPlayingChanged ::: " << _effect->isPlaying();

    if (!_effect->isPlaying()) {
        _next();
    }
}

void AudioOutput::_effectStateChanged()
{
    qDebug() << "_effectStateChanged ::: " << _effect->status();

    if (_effect->status() == QSoundEffect::Ready) {
        _effect->play();
    }
}

void AudioOutput::_timerSlot()
{
    _effect->play();
}

void AudioOutput::_playAfter(int interval)
{
    QTimer::singleShot(interval, this, &AudioOutput::_timerSlot);
}

void AudioOutput::_next() {
    if (_texts.size()) {
        QString text = _texts.first();
        _texts.removeFirst();
        qDebug() << "say ::: " << text;
        _play(text);
    }
}

void AudioOutput::_play(QString& text) {
    qDebug() << "_play ::: " << text;

    for (const QString& key: _audioMap.keys()) {
        qDebug() << "wav ::: " << key;

        if (text.contains(key) || QString::compare(text, key, Qt::CaseInsensitive) == 0) {
            qDebug() << "contains wav ::: " << text;
            QString wav = _audioMap.value(key).value<QString>();
            //QUrl url("qrc:/audio/" + wav);
            qDebug() << "url wav ::: " << wav;

            //QSound::play(url.toString());
            _tts->say(wav);
            return;
        }
    }
};

bool AudioOutput::getMillisecondString(const QString& string, QString& match, int& number) {
    static QRegularExpression re("([0-9]+ms)");
    QRegularExpressionMatchIterator i = re.globalMatch(string);
    while (i.hasNext()) {
        QRegularExpressionMatch qmatch = i.next();
        if (qmatch.hasMatch()) {
            match = qmatch.captured(0);
            number = qmatch.captured(0).replace("ms", "").toInt();
            return true;
        }
    }
    return false;
}

QString AudioOutput::fixTextMessageForAudio(const QString& string) {
    QString match;
    QString newNumber;
    QString result = string;
    qDebug() << "fixTextMessageForAudio ::: " << string;

    //-- Look for codified terms
    if(result.contains("PROXIMITY", Qt::CaseInsensitive)) {
        if (!(QString::compare(result, "AvoidProximity High", Qt::CaseInsensitive) == 0 ||
            QString::compare(result, "AvoidProximity Low", Qt::CaseInsensitive) == 0))
            result = "Proximity Error";
    }
    qDebug() << "result ::: " << result;

    if(result.contains("ERR ", Qt::CaseInsensitive)) {
        result.replace("ERR ", "error ", Qt::CaseInsensitive);
    }
    if(result.contains("ERR:", Qt::CaseInsensitive)) {
        result.replace("ERR:", "error.", Qt::CaseInsensitive);
    }
    if(result.contains("POSCTL", Qt::CaseInsensitive)) {
        result.replace("POSCTL", "Position Control", Qt::CaseInsensitive);
    }
    if(result.contains("ALTCTL", Qt::CaseInsensitive)) {
        result.replace("ALTCTL", "Altitude Control", Qt::CaseInsensitive);
    }
    if(result.contains("AUTO_RTL", Qt::CaseInsensitive)) {
        result.replace("AUTO_RTL", "auto Return To Launch", Qt::CaseInsensitive);
    } else if(result.contains("RTL", Qt::CaseInsensitive)) {
        result.replace("RTL", "Return To Launch", Qt::CaseInsensitive);
    }
    if(result.contains("ACCEL ", Qt::CaseInsensitive)) {
        result.replace("ACCEL ", "accelerometer ", Qt::CaseInsensitive);
    }
    if(result.contains("RC_MAP_MODE_SW", Qt::CaseInsensitive)) {
        result.replace("RC_MAP_MODE_SW", "RC mode switch", Qt::CaseInsensitive);
    }
    if(result.contains("REJ.", Qt::CaseInsensitive)) {
        result.replace("REJ.", "Rejected", Qt::CaseInsensitive);
    }
    if(result.contains("WP", Qt::CaseInsensitive)) {
        result.replace("WP", "way point", Qt::CaseInsensitive);
    }
    if(result.contains("CMD", Qt::CaseInsensitive)) {
        result.replace("CMD", "command", Qt::CaseInsensitive);
    }
    if(result.contains("COMPID", Qt::CaseInsensitive)) {
        result.replace("COMPID", "component eye dee", Qt::CaseInsensitive);
    }
    if(result.contains(" params ", Qt::CaseInsensitive)) {
        result.replace(" params ", " parameters ", Qt::CaseInsensitive);
    }
    if(result.contains(" id ", Qt::CaseInsensitive)) {
        result.replace(" id ", " eye dee ", Qt::CaseInsensitive);
    }
    if(result.contains(" ADSB ", Qt::CaseInsensitive)) {
        result.replace(" ADSB ", " Hey Dee Ess Bee ", Qt::CaseInsensitive);
    }
    if(result.contains(" EKF ", Qt::CaseInsensitive)) {
        result.replace(" EKF ", " Eee Kay Eff ", Qt::CaseInsensitive);
    }
    if(result.contains("PREARM", Qt::CaseInsensitive)) {
        result.replace("PREARM", "pre arm", Qt::CaseInsensitive);
    }
    if(result.contains("PITOT", Qt::CaseInsensitive)) {
        result.replace("PITOT", "pee toe", Qt::CaseInsensitive);
    }

    // Convert negative numbers
    QRegularExpression re(QStringLiteral("(-)[0-9]*\\.?[0-9]"));
    QRegularExpressionMatch reMatch = re.match(result);
    while (reMatch.hasMatch()) {
        if (!reMatch.captured(1).isNull()) {
            // There is a negative prefix
            result.replace(reMatch.capturedStart(1), reMatch.capturedEnd(1) - reMatch.capturedStart(1), tr(" negative "));
        }
        reMatch = re.match(result);
    }

    // Convert real number with decimal point
    re.setPattern(QStringLiteral("([0-9]+)(\\.)([0-9]+)"));
    reMatch = re.match(result);
    while (reMatch.hasMatch()) {
        if (!reMatch.captured(2).isNull()) {
            // There is a decimal point
            result.replace(reMatch.capturedStart(2), reMatch.capturedEnd(2) - reMatch.capturedStart(2), tr(" point "));
        }
        reMatch = re.match(result);
    }

    // Convert meter postfix after real number
    re.setPattern(QStringLiteral("[0-9]*\\.?[0-9]\\s?(m)([^A-Za-z]|$)"));
    reMatch = re.match(result);
    while (reMatch.hasMatch()) {
        if (!reMatch.captured(1).isNull()) {
            // There is a meter postfix
            result.replace(reMatch.capturedStart(1), reMatch.capturedEnd(1) - reMatch.capturedStart(1), tr(" meters"));
        }
        reMatch = re.match(result);
    }

    int number;
    if(getMillisecondString(string, match, number) && number > 1000) {
        if(number < 60000) {
            int seconds = number / 1000;
            newNumber = QString("%1 second%2").arg(seconds).arg(seconds > 1 ? "s" : "");
        } else {
            int minutes = number / 60000;
            int seconds = (number - (minutes * 60000)) / 1000;
            if (!seconds) {
                newNumber = QString("%1 minute%2").arg(minutes).arg(minutes > 1 ? "s" : "");
            } else {
                newNumber = QString("%1 minute%2 and %3 second%4").arg(minutes).arg(minutes > 1 ? "s" : "").arg(seconds).arg(seconds > 1 ? "s" : "");
            }
        }
        result.replace(match, newNumber);
    }
    return result;
}
