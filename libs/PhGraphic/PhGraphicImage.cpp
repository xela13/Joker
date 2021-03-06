/**
 * @file
 * @copyright (C) 2012-2014 Phonations
 * @license http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#include <QImage>

#include "PhTools/PhDebug.h"
#include "PhGraphicImage.h"

PhGraphicImage::PhGraphicImage(QString filename, int x, int y, int w, int h)
	: PhGraphicTexturedRect(x, y, w, h), _filename(filename)
{
}

bool PhGraphicImage::init()
{
	QImage image;
	if(image.load(_filename)) {
		if (image.format() == QImage::Format::Format_Indexed8) {
			image = image.convertToFormat(QImage::Format::Format_ARGB32);
			PHDEBUG << "Converting to format:" << image.format();
		}
		if(createTextureFromARGBBuffer(image.bits(), image.width(), image.height())) {
			_originalSize.setWidth(image.width());
			_originalSize.setHeight(image.height());
			return PhGraphicTexturedRect::init();
		}
		else {
			PHDEBUG << "Error creating buffer for " << _filename;
		}
	}
	else {
		PHDEBUG << "Error loading" << _filename;
	}

	return false;
}

QSize PhGraphicImage::originalSize() const
{
	return _originalSize;
}

void PhGraphicImage::draw()
{
	if(!_filename.isEmpty() && !this->ready())
		this->init();

	PhGraphicTexturedRect::draw();
}

void PhGraphicImage::setFilename(QString filename)
{
	_filename = filename;
}

QString PhGraphicImage::fileName()
{
	return _filename;
}
