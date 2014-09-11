/**
 * @file
 * @copyright (C) 2012-2014 Phonations
 * @license http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QListWidgetItem>
#include <QButtonGroup>

#include "JokerSettings.h"

namespace Ui {
class PreferencesDialog;
}
/**
 * @brief Joker preferences dialog
 *
 * The dialog contains the following user settings:
 * - General HUD: enable/disable element like current timecode
 * - Synchronization: delay, sync mode, sync mode configuration
 * - PhGraphicStrip setting: boldness, speed, font, height
 * - Log
 */
class PreferencesDialog : public QDialog
{
	Q_OBJECT

public:
	/**
	 * @brief The PreferencesDialog constructor
	 *
	 * @param settings The application settings
	 * @param parent The parent object
	 */
	explicit PreferencesDialog(JokerSettings *settings, QWidget *parent = 0);
	~PreferencesDialog();

protected:
	/**
	 * @brief Update the settings according to the UI
	 */
	void accept();

	/**
	 * @brief Revert the settings to the old values
	 */
	void reject();

private slots:
	void updateSynchronisationEnabledControl(int, bool);

	void on_spinBoxDelay_valueChanged(int delay);
	void on_radioButtonQF_toggled(bool checked);
	void on_sliderStripHeight_valueChanged(int position);
	void on_spinBoxSpeed_valueChanged(int speed);
	void on_listWidgetFont_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);

	void on_sliderBoldness_valueChanged(int value);

	void on_lineEditFilter_textEdited(const QString &value);

	void on_cBoxDisplayTC_clicked();

	void on_cBoxDisplayNextTC_clicked();

	void on_cBoxDisplayNextText_clicked();

	void on_cBoxDisplayTitle_clicked();

	void on_cBoxDisplayLoop_clicked();

private:
	Ui::PreferencesDialog *ui;
	JokerSettings *_settings;
	QString _oldFont;
	bool _oldUseQuarterFrame;
	int _oldDelay;
	int _oldHorizontalTimePerPixel;
	int _oldBolness;
	float _oldStripHeight;
	bool _oldDisplayTC;
	bool _oldDisplayNextTC;
	bool _oldDisplayNextText;
	bool _oldDisplayTitle;
	bool _oldDisplayLoop;

	QMap<QString, QString> _fontList;
	QMap<QString, QString> _langNameMap;

	QButtonGroup _delayButtonGroup, _protocolButtonGroup, _midiPortTypeButtonGroup;
};

#endif // PREFERENCESDIALOG_H
