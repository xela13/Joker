#include "TextEditTestWindow.h"
#include "PhCommonUI/FeedbackReporter.h"
#include <QApplication>

#include "PhTools/PhDebug.h"

int main(int argc, char *argv[])
{
	TextEditTestSettings settings;

	QApplication a(argc, argv);
	FeedbackReporter f(&settings);

	if(!settings.exitedNormaly() or true) {
		f.show();
	}

	settings.setExitedNormaly(false);
	TextEditTestWindow w(&settings);
	w.processArg(argc, argv);
	w.show();


	int result = a.exec();
	settings.setExitedNormaly(result == 0);

	return result;
}
