#include "PhLtcReader.h"

PhLtcReader::PhLtcReader(QObject *parent) :
    QObject(parent),
    _clock(PhTimeCodeType25),
	_input(NULL),
	_position(0),
	_buffer(NULL)
{
    _decoder = ltc_decoder_create(1920, 3840);
    PHDEBUG << "LTC Reader created";
}

bool PhLtcReader::init(QString input)
{
    QList<QAudioDeviceInfo> list = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

	if(list.isEmpty())
	{
		PHDEBUG << "No audio input device";
		return false;
	}

	QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();

    foreach(QAudioDeviceInfo device, list)
    {
        if(device.deviceName() == input)
            info = device;
    }

    PHDEBUG << "LTC input device :" << info.deviceName();

    QAudioFormat format;
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleSize(8);
    format.setSampleType(QAudioFormat::SignedInt);

    if(!info.isFormatSupported(format))
    {
        PHDEBUG << "Unsupported audio format";
        return false;
    }

	_position = 0;
    _input = new QAudioInput(info, format);

    connect(_input, SIGNAL(notify()), this, SLOT(onNotify()));
    _buffer = _input->start();
    _input->setNotifyInterval(10);

	_pauseDetector.start();

	return true;
}

void PhLtcReader::close()
{
	if(_input)
	{
		_input->stop();
		delete _buffer;
		_buffer = NULL;
		delete _input;
		_input = NULL;
	}
}

QList<QString> PhLtcReader::inputList()
{
    QList<QString> names;
    QList<QAudioDeviceInfo> list = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    foreach(QAudioDeviceInfo device, list)
        names.append(device.deviceName());

    return names;
}

PhClock *PhLtcReader::clock()
{
    return &_clock;
}

void PhLtcReader::onNotify()
{
    // Pause if no frame have been received for more than 0.5 sec
    if(_pauseDetector.elapsed() > 500)
        _clock.setRate(0);

    QByteArray array = _buffer->readAll();
    char max = 0;
    for(int i = 0; i < array.count(); i++)
    {
        if(array.at(i) > max)
            max = array.at(i);
    }

    ltc_decoder_write(_decoder, (ltcsnd_sample_t*)array.data(), array.count(), _position);

    LTCFrameExt frame;
    unsigned int hhmmssff[4];
	PhTime newT = 0;
	PhFrame oldFrame = _clock.frame();
	ltc_off_t lastOffset = -1;
    while(ltc_decoder_read(_decoder, &frame))
    {
        hhmmssff[0] = frame.ltc.hours_tens * 10 + frame.ltc.hours_units;
        hhmmssff[1] = frame.ltc.mins_tens * 10 + frame.ltc.mins_units;
        hhmmssff[2] = frame.ltc.secs_tens * 10 + frame.ltc.secs_units;
        hhmmssff[3] = frame.ltc.frame_tens * 10 + frame.ltc.frame_units;

#warning TODO Handle other rate pouiem conversion (issue #43)
        PhFrame newFrame = PhTimeCode::frameFromHhMmSsFf(hhmmssff, PhTimeCodeType25);
		PHDBG(20) << newFrame;
		// Compute the new time : 1 frame = 24 pouiem (at 25 fps)
		newT = newFrame * 24;
		lastOffset = frame.off_start;
        if(oldFrame != newFrame)
        {
            _pauseDetector.restart();
            if(oldFrame < newFrame)
                _clock.setRate(1);
            else
                _clock.setRate(-1);
        }

    }

    _position += array.count();

	if(lastOffset > 0)
	{
		// 1s = 48000 samples
		// 1s = 600 pouiem
		// 1 pouiem = 80 samples
		PhTime timeOffset = (_position - lastOffset) / 80;
		_clock.setTime(newT + timeOffset);
	}
}
