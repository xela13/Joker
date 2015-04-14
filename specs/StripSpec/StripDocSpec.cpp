/**
 * Copyright (C) 2012-2014 Phonations
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 */

#include "PhTools/PhDebug.h"
#include "PhStrip/PhStripDoc.h"

#include "CommonSpec.h"

#include "PhSpec.h"

using namespace bandit;

go_bandit([](){
	describe("strip test", [&]() {
		PhStripDoc doc;

		before_each([&](){
			//PhDebug::disable();
			doc.reset();
		});

		describe("joker", [&]() {
			it("open strip, old joker file", [&](){
				AssertThat(doc.openStripFile("test.strip"), IsTrue());
				AssertThat(doc.forceRatio169(), IsTrue());
				AssertThat(doc.videoFilePath().toStdString(), Equals("test01.mov"));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("00:59:00:00"));
				AssertThat(t2s(doc.lastTime(), PhTimeCodeType25), Equals("01:02:03:04"));
			});

			it("open joker file linking to a detx file", [&](){
				AssertThat(doc.openStripFile("test01.joker"), IsTrue());
				AssertThat(doc.filePath().toStdString(), Equals("test01.detx"));
				AssertThat(doc.videoFilePath().toStdString(), Equals("test01.mov"));
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("01:01:00:00"));
				AssertThat(doc.forceRatio169(), IsTrue());
				AssertThat(t2s(doc.lastTime(), PhTimeCodeType25), Equals("01:30:00:00"));

				AssertThat(doc.title().toStdString(), Equals("Title test"));
				AssertThat(doc.generator().toStdString(), Equals("Cappella v0.12.5, 1"));
			});

			it("open joker file linking to a detx file", [&](){
				AssertThat(doc.openStripFile("test02.joker"), IsTrue());
				AssertThat(doc.filePath().toStdString(), Equals("test03.mos"));
				AssertThat(doc.videoFilePath().toStdString(), Equals("test02.mov"));
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("01:03:00:00"));
				AssertThat(doc.forceRatio169(), IsFalse());
				AssertThat(t2s(doc.lastTime(), PhTimeCodeType25), Equals("01:20:00:00"));

				AssertThat(doc.title().toStdString(), Equals("Au fil d'Ariane"));
				AssertThat(doc.generator().toStdString(), Equals("Mosaic"));
			});

			it("fails to open bad tag joker file", [&](){
				AssertThat(QFile::exists("bad_tag.joker"), IsTrue());
				AssertThat(doc.openStripFile("bad_tag.joker"), IsFalse());

				AssertThat(QFile::exists("empty.joker"), IsTrue());
				AssertThat(doc.openStripFile("empty.joker"), IsFalse());
			});

			it("can open well formed empty file", [&](){
				AssertThat(QFile::exists("empty_root.joker"), IsTrue());
				AssertThat(doc.openStripFile("empty_root.joker"), IsTrue());

				AssertThat(QFile::exists("empty_meta.joker"), IsTrue());
				AssertThat(doc.openStripFile("empty_meta.joker"), IsTrue());
			});

			#warning /// @todo Test video frame rate

			it("open and save with detx", [&](){
				AssertThat(doc.openStripFile("test01.joker"), IsTrue());

				AssertThat(doc.saveStripFile("save01.joker", s2t("01:01:01:01", PhTimeCodeType25)), IsTrue());

				doc.reset();

				AssertThat(doc.openStripFile("save01.joker"), IsTrue());

				AssertThat(doc.filePath().toStdString(), Equals("test01.detx"));
				AssertThat(doc.videoFilePath().toStdString(), Equals("test01.mov"));
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("01:01:00:00"));
				AssertThat(doc.forceRatio169(), IsTrue());
				AssertThat(doc.videoDeinterlace(), IsFalse());
				AssertThat(t2s(doc.lastTime(), PhTimeCodeType25), Equals("01:01:01:01"));

				AssertThat(doc.title().toStdString(), Equals("Title test"));
				AssertThat(doc.generator().toStdString(), Equals("Cappella v0.12.5, 1"));
			});

			it("open and save with mos", [&](){
				AssertThat(doc.openStripFile("test02.joker"), IsTrue());

				AssertThat(doc.saveStripFile("save02.joker", s2t("02:02:02:02", PhTimeCodeType25)), IsTrue());

				AssertThat(doc.openStripFile("save02.joker"), IsTrue());

				AssertThat(doc.filePath().toStdString(), Equals("test03.mos"));
				AssertThat(doc.videoFilePath().toStdString(), Equals("test02.mov"));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("01:03:00:00"));
				AssertThat(doc.forceRatio169(), IsFalse());
				AssertThat(doc.videoDeinterlace(), IsTrue());
				AssertThat(t2s(doc.lastTime(), PhTimeCodeType25), Equals("02:02:02:02"));

				AssertThat(doc.title().toStdString(), Equals("Au fil d'Ariane"));
				AssertThat(doc.generator().toStdString(), Equals("Mosaic"));
			});
		});

		describe("detx", [&]() {
			describe("import test01.detx", [&]() {
				before_each([&](){
					AssertThat(doc.importDetXFile("test01.detx"), IsTrue());
				});

				it("import header", [&](){
					AssertThat(t2s(doc.lastTime(), PhTimeCodeType25), Equals("01:00:16:00"));
					AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("01:00:00:00"));
				});

				it("import_text", [&](){
					AssertThat(doc.texts().count(), Equals(6));

					AssertThat(doc.texts()[0]->content().toStdString(), Equals("Simple sentence"));
					AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:02:00"));
					AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType25), Equals("01:00:04:00"));
					AssertThat(doc.texts()[0]->people(), Equals(doc.peopleByName("Jeanne")));
					AssertThat(doc.texts()[0]->y(), Equals(0.25f));

					AssertThat(doc.texts()[1]->content().toStdString(), Equals("Composed "));
					AssertThat(t2s(doc.texts()[1]->timeIn(), PhTimeCodeType25), Equals("01:00:05:00"));
					AssertThat(t2s(doc.texts()[1]->timeOut(), PhTimeCodeType25), Equals("01:00:06:00"));

					AssertThat(doc.texts()[2]->content().toStdString(), Equals("sentence"));
					AssertThat(t2s(doc.texts()[2]->timeIn(), PhTimeCodeType25), Equals("01:00:06:00"));
					AssertThat(t2s(doc.texts()[2]->timeOut(), PhTimeCodeType25), Equals("01:00:07:00"));

					AssertThat(doc.texts()[3]->content().toStdString(), Equals("Simple off sentence"));
					AssertThat(t2s(doc.texts()[3]->timeIn(), PhTimeCodeType25), Equals("01:00:12:00"));
					AssertThat(t2s(doc.texts()[3]->timeOut(), PhTimeCodeType25), Equals("01:00:14:00"));

					AssertThat(doc.texts()[4]->content().toStdString(), Equals("Composed sentence with off"));
					AssertThat(t2s(doc.texts()[4]->timeIn(), PhTimeCodeType25), Equals("01:00:15:00"));
					AssertThat(t2s(doc.texts()[4]->timeOut(), PhTimeCodeType25), Equals("01:00:17:00"));

					AssertThat(doc.texts()[5]->content().toStdString(), Equals("Sentence with out not linked"));
					AssertThat(t2s(doc.texts()[5]->timeIn(), PhTimeCodeType25), Equals("01:00:30:00"));
					AssertThat(t2s(doc.texts()[5]->timeOut(), PhTimeCodeType25), Equals("01:00:31:04"));
				});

				it("import detect", [&](){
					AssertThat(doc.detects().count(), Equals(6));

					AssertThat(doc.detects()[0]->people(), Equals(doc.peopleByName("Jeanne")));
					AssertThat(doc.detects()[0]->type(), Equals(PhStripDetect::On));
					AssertThat(t2s(doc.detects()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:02:00"));
					AssertThat(t2s(doc.detects()[0]->timeOut(), PhTimeCodeType25), Equals("01:00:04:00"));
					AssertThat(doc.detects()[0]->y(), Equals(0.25f));

					AssertThat(doc.detects()[1]->people(), Equals(doc.peopleByName("Sue")));
					AssertThat(doc.detects()[1]->type(), Equals(PhStripDetect::On));
					AssertThat(t2s(doc.detects()[1]->timeIn(), PhTimeCodeType25), Equals("01:00:05:00"));
					AssertThat(t2s(doc.detects()[1]->timeOut(), PhTimeCodeType25), Equals("01:00:07:00"));
					AssertThat(doc.detects()[1]->y(), Equals(0.5f));


					AssertThat(doc.detects()[2]->people(), Equals(doc.peopleByName("Paul")));
					AssertThat(doc.detects()[2]->type(), Equals(PhStripDetect::Off));
					AssertThat(t2s(doc.detects()[2]->timeIn(), PhTimeCodeType25), Equals("01:00:12:00"));
					AssertThat(t2s(doc.detects()[2]->timeOut(), PhTimeCodeType25), Equals("01:00:14:00"));
					AssertThat(doc.detects()[2]->y(), Equals(0.25f));


					AssertThat(doc.detects()[3]->people(), Equals(doc.peopleByName("Sue")));
					AssertThat(doc.detects()[3]->type(), Equals(PhStripDetect::Off));
					AssertThat(t2s(doc.detects()[3]->timeIn(), PhTimeCodeType25), Equals("01:00:15:00"));
					AssertThat(t2s(doc.detects()[3]->timeOut(), PhTimeCodeType25), Equals("01:00:17:00"));
					AssertThat(doc.detects()[3]->y(), Equals(0.5f));

					AssertThat(doc.detects()[4]->people(), Equals(doc.peopleByName("Sue")));
					AssertThat(doc.detects()[4]->type(), Equals(PhStripDetect::Off));
					AssertThat(t2s(doc.detects()[4]->timeIn(), PhTimeCodeType25), Equals("01:00:20:00"));
					AssertThat(t2s(doc.detects()[4]->timeOut(), PhTimeCodeType25), Equals("01:00:22:00"));
					AssertThat(doc.detects()[4]->y(), Equals(0.5f));


					AssertThat(doc.detects()[5]->people(), Equals(doc.peopleByName("Sue")));
					AssertThat(doc.detects()[5]->type(), Equals(PhStripDetect::On));
					AssertThat(t2s(doc.detects()[5]->timeIn(), PhTimeCodeType25), Equals("01:00:30:00"));
					AssertThat(t2s(doc.detects()[5]->timeOut(), PhTimeCodeType25), Equals("01:00:31:04"));
					AssertThat(doc.detects()[5]->y(), Equals(0.5f));
				});
			});

			describe("import test01.detx", [&]() {
				before_each([&](){
					doc.reset();
				});

				it("compute detx id", [&](){
					AssertThat(doc.computeDetXId("abc").toStdString(), Equals("abc"));
					AssertThat(doc.computeDetXId("ABC").toStdString(), Equals("abc"));
					AssertThat(doc.computeDetXId("It's ok!").toStdString(), Equals("it_s_ok_"));
				});

				it("import detx without title", [&](){
					AssertThat(doc.importDetXFile("notitle.detx"), IsTrue());
					AssertThat(doc.title().toStdString(), Equals("notitle"));
				});

				it("export and import basic detx", [&](){
					doc.setTitle("Title test for detx");
					doc.setVideoFilePath("test01.mov");
					doc.setVideoTimeIn(s2t25("01:01:00:00"), PhTimeCodeType25);
					PhPeople *bob = new PhPeople("Bob", "#0000ff", "not base64 data, just for test");
					doc.addPeople(bob);
					PhPeople *sue = new PhPeople("Sue", "#ff00ff", "bouboubou");
					doc.addPeople(sue);

					doc.addObject(new PhStripLoop(s2t25("01:01:00:00"), "1"));

					doc.addObject(new PhStripCut(s2t25("01:01:00:02"), PhStripCut::Simple));

					doc.addObject(new PhStripText(s2t25("01:01:00:05"), sue, s2t25("01:01:00:15"), 0.50f, "Hello", 0.25f));

					AssertThat(doc.exportDetXFile("save01.detx", s2t25("01:01:01:01")), IsTrue());

					doc.reset();

					AssertThat(doc.importDetXFile("save01.detx"), IsTrue());

					AssertThat(doc.filePath().toStdString(), Equals("save01.detx"));
					AssertThat(doc.title().toStdString(), Equals("Title test for detx"));
					AssertThat(doc.videoFilePath().toStdString(), Equals("test01.mov"));
					AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
					AssertThat(t2s25(doc.videoTimeIn()), Equals("01:01:00:00"));
					AssertThat(doc.forceRatio169(), IsFalse());
					AssertThat(doc.videoDeinterlace(), IsFalse());
					AssertThat(t2s25(doc.lastTime()), Equals("01:01:01:01"));

					// Peoples
					AssertThat(doc.peoples().count(), Equals(2));

					AssertThat(doc.peoples().at(0)->name().toStdString(), Equals("Bob"));
					AssertThat(doc.peoples().at(0)->color().toStdString(), Equals("#0000ff"));
					AssertThat(doc.peoples().at(0)->picture().toStdString(), Equals("not base64 data, just for test"));

					AssertThat(doc.peoples().at(1)->name().toStdString(), Equals("Sue"));
					AssertThat(doc.peoples().at(1)->color().toStdString(), Equals("#ff00ff"));
					AssertThat(doc.peoples().at(1)->picture().toStdString(), Equals("bouboubou"));

					// Loops
					AssertThat(doc.loops().count(), Equals(1));

					PhStripLoop *loop = doc.loops().at(0);
					AssertThat(t2s25(loop->timeIn()), Equals("01:01:00:00"));
					AssertThat(loop->label().toStdString(), Equals("1"));

					// Cuts
					AssertThat(doc.cuts().count(), Equals(1));

					PhStripCut *cut = doc.cuts().at(0);
					AssertThat(t2s25(cut->timeIn()), Equals("01:01:00:02"));
					AssertThat(cut->type(), Equals(PhStripCut::Simple));

					// Texts
					AssertThat(doc.texts().count(), Equals(1));

					// simple text
					PhStripText *text = doc.texts().at(0);
					AssertThat(t2s25(text->timeIn()), Equals("01:01:00:05"));
					AssertThat(text->people()->name().toStdString(), Equals("Sue"));
					AssertThat(t2s25(text->timeOut()), Equals("01:01:00:15"));
					AssertThat(text->content().toStdString(), Equals("Hello"));
				});

				it("export and import detx with complex lines", [&](){
					doc.setTitle("export and import detx with complex lines");
					PhPeople *sue = new PhPeople("Sue", "#ff00ff", "bouboubou");
					doc.addPeople(sue);

					doc.addObject(new PhStripText(s2t25("01:01:00:05"), sue, s2t25("01:01:00:15"), 0.50f, "Hello ", 0.25f));
					doc.addObject(new PhStripText(s2t25("01:01:00:15"), sue, s2t25("01:01:01:00"), 0.50f, "world", 0.25f));

					AssertThat(doc.exportDetXFile("save02.detx", s2t25("01:01:01:01")), IsTrue());

					doc.reset();

					AssertThat(doc.importDetXFile("save02.detx"), IsTrue());

					AssertThat(doc.filePath().toStdString(), Equals("save02.detx"));
					AssertThat(doc.title().toStdString(), Equals("export and import detx with complex lines"));

					// Texts
					AssertThat(doc.texts().count(), Equals(2));

					// double text
					PhStripText *text = doc.texts().at(0);
					AssertThat(t2s25(text->timeIn()), Equals("01:01:00:05"));
					AssertThat(text->people()->name().toStdString(), Equals("Sue"));
					AssertThat(t2s25(text->timeOut()), Equals("01:01:00:15"));
					AssertThat(text->content().toStdString(), Equals("Hello "));

					text = doc.texts().at(1);
					AssertThat(t2s25(text->timeIn()), Equals("01:01:00:15"));
					AssertThat(text->people()->name().toStdString(), Equals("Sue"));
					AssertThat(t2s25(text->timeOut()), Equals("01:01:01:00"));
					AssertThat(text->content().toStdString(), Equals("world"));
				});
			});
		});

		describe("mos", [&]() {
			it("import_test01", [&]() {
				PhStripDoc doc;
				AssertThat(doc.importMosFile("test01.mos"), IsTrue());

				AssertThat(doc.authorName().toStdString(), Equals("auteur"));

				AssertThat(doc.videoFilePath().toStdString(), Equals(""));
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("00:00:00:00"));

				AssertThat(doc.title().toStdString(), Equals("Titre VO"));
				AssertThat(doc.season().toStdString(), Equals("saison"));
				AssertThat(doc.episode().toStdString(), Equals("episode"));

				// Test peoples
				AssertThat(doc.peoples().count(), Equals(1));
				PhPeople * people = doc.peopleByName("Nom personnage");
				AssertThat(people != NULL, IsTrue());
				AssertThat(people->name().toStdString(), Equals("Nom personnage"));

				// Test cuts
				AssertThat(doc.cuts().count(), Equals(0));

				// Test loop
				AssertThat(doc.loops().count(), Equals(0));

				// Test texts
				AssertThat(doc.texts().count(), Equals(3));
				AssertThat(doc.texts()[0]->people(), Equals(people));
				AssertThat(doc.texts()[0]->content().toStdString(), Equals("Ceci "));
				AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType25), Equals("00:00:00:20"));
				AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType25), Equals("00:00:01:12"));

				AssertThat(doc.texts()[1]->content().toStdString(), Equals("est un"));
				AssertThat(t2s(doc.texts()[1]->timeIn(), PhTimeCodeType25), Equals("00:00:01:12"));
				AssertThat(t2s(doc.texts()[1]->timeOut(), PhTimeCodeType25), Equals("00:00:01:16"));

				AssertThat(doc.texts()[2]->content().toStdString(), Equals(" test."));
				AssertThat(t2s(doc.texts()[2]->timeIn(), PhTimeCodeType25), Equals("00:00:01:16"));
				AssertThat(t2s(doc.texts()[2]->timeOut(), PhTimeCodeType25), Equals("00:00:02:03"));

				// Detect test
				//#warning TODO more test on detect
				AssertThat(doc.detects().count(), Equals(4));
			});

			it("import_test02", [&]() {
				PhStripDoc doc;
				AssertThat(doc.importMosFile("test02.mos"), IsTrue());

				AssertThat(doc.videoFilePath().toStdString(), Equals("C:\\Users\\Matthhou\\Desktop\\Burn Notice\\710\\BurnNotice_BCI710_VOVI.mov"));
#warning TODO Matthias told me that the timestamp was in fact 00:58:00:00...
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("00:58:24:00"));

				// Test peoples
				AssertThat(doc.peoples().count(), Equals(2));

				PhPeople * pierre = doc.peopleByName("Pierre");
				AssertThat(pierre != NULL, IsTrue());
				AssertThat(pierre->name().toStdString(), Equals("Pierre"));

				PhPeople * marie = doc.peopleByName("Marie");
				AssertThat(marie != NULL, IsTrue());
				AssertThat(marie->name().toStdString(), Equals("Marie"));

				// Test cuts
				AssertThat(doc.cuts().count(), Equals(1));
				AssertThat(t2s(doc.cuts()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:00:00"));

				// Test loops
				AssertThat(doc.loops().count(), Equals(1));
				AssertThat(t2s(doc.loops()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:00:00"));
				AssertThat(doc.loops()[0]->label().toStdString(), Equals("1"));

				// Test texts
				AssertThat(doc.texts().count(), Equals(2));
				AssertThat(doc.texts()[0]->people(), Equals(pierre));
				AssertThat(doc.texts()[0]->y(), Equals(0.0f));
				AssertThat(doc.texts()[0]->content().toStdString(), Equals("Bonjour, Marie."));
				AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:00:00"));
				AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType25), Equals("01:00:02:00"));

				AssertThat(doc.texts()[1]->people(), Equals(marie));
				AssertThat(doc.texts()[1]->y(), Equals(0.6f));
				AssertThat(doc.texts()[1]->content().toStdString(), Equals("Bonjour, Pierre."));
				AssertThat(t2s(doc.texts()[1]->timeIn(), PhTimeCodeType25), Equals("01:00:04:00"));
				AssertThat(t2s(doc.texts()[1]->timeOut(), PhTimeCodeType25), Equals("01:00:06:00"));

				// Detect test
				AssertThat(doc.detects().count(), Equals(0));
			});

			it("import_test03", [&]() {
				PhStripDoc doc;
				AssertThat(doc.importMosFile("test03.mos"), IsTrue());

				AssertThat(doc.videoFilePath().toStdString(), Equals("Z:\\MOT POUR MO\\AU FIL D'ARIANE_DETECTION\\jpegAFA_BOB 06_SEQ 30_PISTES SEPARES_H264.mov"));
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType24));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType24), Equals("05:59:50:00"));

				// Test peoples
				AssertThat(doc.peoples().count(), Equals(2));

				PhPeople * noName = doc.peopleByName("");
				AssertThat(noName != NULL, IsTrue());
				AssertThat(noName->name().toStdString(), Equals(""));

				PhPeople * denis = doc.peopleByName("Denis");
				AssertThat(denis != NULL, IsTrue());
				AssertThat(denis->name().toStdString(), Equals("Denis"));

				// Test cuts
				AssertThat(doc.cuts().count(), Equals(29));
				AssertThat(t2s(doc.cuts()[0]->timeIn(), PhTimeCodeType24), Equals("05:59:50:00"));
				AssertThat(t2s(doc.cuts()[1]->timeIn(), PhTimeCodeType24), Equals("05:59:58:00"));
				AssertThat(t2s(doc.cuts()[2]->timeIn(), PhTimeCodeType24), Equals("06:00:00:00"));
				AssertThat(t2s(doc.cuts()[3]->timeIn(), PhTimeCodeType24), Equals("06:00:05:22"));
				AssertThat(t2s(doc.cuts()[4]->timeIn(), PhTimeCodeType24), Equals("06:00:08:05"));

				// Test loops
				AssertThat(doc.loops().count(), Equals(2));
				AssertThat(t2s(doc.loops()[0]->timeIn(), PhTimeCodeType24), Equals("06:00:01:00"));
				AssertThat(doc.loops()[0]->label().toStdString(), Equals("30"));
				AssertThat(t2s(doc.loops()[1]->timeIn(), PhTimeCodeType24), Equals("06:01:15:00"));
				AssertThat(doc.loops()[1]->label().toStdString(), Equals("off"));

				// Test texts
				AssertThat(doc.texts(false).count(), Equals(206));
				AssertThat(doc.texts(true).count(), Equals(0));

				AssertThat(doc.texts()[0]->people(), Equals(noName));
				AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType24), Equals("05:59:49:05"));
				AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType24), Equals("05:59:49:20"));

				AssertThat(doc.texts()[0]->y(), Equals(2.0f));
				AssertThat(doc.texts()[0]->height(), Equals(0.2f));

				AssertThat(doc.texts()[0]->content().toStdString(), Equals("Départ bob 6"));
				AssertThat(doc.texts()[1]->content().toStdString(), Equals("(X)"));
				AssertThat(doc.texts()[2]->content().toStdString(), Equals("05.59.50.00"));
				AssertThat(doc.texts()[3]->content().toStdString(), Equals("1000"));
				AssertThat(doc.texts()[4]->content().toStdString(), Equals("(X)"));
				AssertThat(doc.texts()[5]->content().toStdString(), Equals("P.I"));

				AssertThat(doc.texts()[9]->people(), Equals(denis));
				AssertThat(t2s(doc.texts()[9]->timeIn(), PhTimeCodeType24), Equals("06:00:07:23"));
				AssertThat(t2s(doc.texts()[9]->timeOut(), PhTimeCodeType24), Equals("06:00:08:03"));
				AssertThat(t2s(doc.texts()[10]->timeIn(), PhTimeCodeType24), Equals("06:00:08:03"));
				AssertThat(t2s(doc.texts()[10]->timeOut(), PhTimeCodeType24), Equals("06:00:08:07"));

				AssertThat(doc.texts()[9]->content().toStdString(), Equals("Tu "));
				AssertThat(doc.texts()[10]->content().toStdString(), Equals("tra"));
				AssertThat(doc.texts()[11]->content().toStdString(), Equals("vaillais "));
				AssertThat(doc.texts()[12]->content().toStdString(), Equals("pas "));
				AssertThat(doc.texts()[13]->content().toStdString(), Equals("ce "));
				AssertThat(doc.texts()[14]->content().toStdString(), Equals("soir !"));

				AssertThat(t2s(doc.texts()[14]->timeOut(), PhTimeCodeType24), Equals("06:00:09:06"));

				// Detect test
				AssertThat(doc.detects().count(), Equals(24));
				//#warning TODO more test on detect
			});

			it("import_test04", [&]() {
				PhStripDoc doc;

				AssertThat(doc.importMosFile("test04.mos"), IsTrue());

				// Test video info
				AssertThat(doc.videoFilePath().toStdString(), Equals("D:\\Ressources\\Mosaic\\Utilisateurs\\Yves\\Bold 5704\\5704.mov"));
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("00:59:39:24"));

				// Test loops
				AssertThat(doc.loops().count(), Equals(27));
				AssertThat(t2s(doc.loops()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:00:13"));
				AssertThat(doc.loops()[0]->label().toStdString(), Equals("1"));
				AssertThat(doc.loops()[1]->label().toStdString(), Equals("2"));
				AssertThat(doc.loops()[2]->label().toStdString(), Equals("3"));
				AssertThat(doc.loops()[3]->label().toStdString(), Equals("off"));
				AssertThat(doc.loops()[4]->label().toStdString(), Equals("4"));
				AssertThat(t2s(doc.loops()[5]->timeIn(), PhTimeCodeType25), Equals("01:02:56:02"));
				AssertThat(doc.loops()[5]->label().toStdString(), Equals("5"));
				AssertThat(t2s(doc.loops()[25]->timeIn(), PhTimeCodeType25), Equals("01:18:14:11"));
				AssertThat(doc.loops()[25]->label().toStdString(), Equals("25"));
				AssertThat(t2s(doc.loops()[26]->timeIn(), PhTimeCodeType25), Equals("01:19:01:01"));
				AssertThat(doc.loops()[26]->label().toStdString(), Equals("off"));

				// Test texts
				AssertThat(doc.texts().count(), Equals(1118));
				AssertThat(doc.texts(true).count(), Equals(4329));
				AssertThat(doc.detects().count(), Equals(4552));

				AssertThat(t2s(doc.detects()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:01:13"));
				AssertThat(t2s(doc.detects()[0]->timeOut(), PhTimeCodeType25), Equals("01:00:02:20"));
				AssertThat(doc.detects()[0]->type(), Equals(PhStripDetect::Off));
			});

			it("import_mos24", [&]() {
				PhStripDoc doc;

				AssertThat(doc.importMosFile("mos24.mos"), IsTrue());

				AssertThat(doc.videoFilePath().toStdString(), Equals("C:\\Users\\Gilles\\Desktop\\Sonic_EP_01_mix_VA.mov"));
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType24));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType24), Equals("00:59:58:00"));

				AssertThat(doc.texts().count(), Equals(1));

				AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType24), Equals("01:00:01:00"));
				AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType24), Equals("01:00:03:00"));
			});

			it("import_mos25", [&]() {
				PhStripDoc doc;

				AssertThat(doc.importMosFile("mos25.mos"), IsTrue());

				AssertThat(doc.videoFilePath().toStdString(), Equals("C:\\Users\\Gilles\\Desktop\\Get Blake\\Get Blake 115\\GBL_EP115_Online_Master_VA_h264_TCI.mov"));
				AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
				AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("09:59:58:00"));
			});
		});

		describe("drb", [&]() {
			describe("v5", [&]() {
				it("import_drb01", [&]() {
					AssertThat(doc.openStripFile("drb01.drb"), IsTrue());

					AssertThat(doc.videoFilePath().toStdString(), Equals("C:\\Users\\SYNCHRO USER\\Downloads\\TheManWithTheGoldenArm_25fps_high\\TheManWithTheGoldenArm_25fps_high.mov"));

					AssertThat(doc.loops().count(), Equals(0));

					AssertThat(doc.peoples().count(), Equals(2));
					AssertThat(doc.peoples()[0]->name().toStdString(), Equals("Personnage 1"));
					AssertThat(doc.peoples()[1]->name().toStdString(), Equals("Personnage 2"));

					AssertThat(doc.texts().count(), Equals(1));
					AssertThat(doc.texts()[0]->people()->name().toStdString(), Equals("Personnage 2"));

					AssertThat(doc.texts()[0]->y(), Equals(0.36f));
					AssertThat(doc.texts()[0]->height(), EqualsWithDelta(0.22666667f, 0.00001f));

#warning /// @todo should be the same than syn6
					AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType25), Equals("00:02:12:10"));
					AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType25), Equals("00:02:44:07"));
				});

				it("import_drb02", [&]() {
					AssertThat(doc.openStripFile("drb02.drb"), IsTrue());

					AssertThat(doc.videoFilePath().toStdString(), Equals("D:\\NED 201.mov"));
					AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
					AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("00:58:04:20"));

					AssertThat(doc.loops().count(), Equals(21));

					for(int i = 0; i < 21; i++)
						AssertThat(doc.loops().at(i)->label().toStdString(), Equals(QString::number(i + 1).toStdString()));

					AssertThat(t2s(doc.loops()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:39:02"));
					AssertThat(t2s(doc.loops()[1]->timeIn(), PhTimeCodeType25), Equals("01:02:14:23"));

					AssertThat(doc.peoples().count(), Equals(28));

					AssertThat(doc.peoples()[0]->name().toStdString(), Equals("Intervenant 1"));
					AssertThat(doc.peoples()[1]->name().toStdString(), Equals("ned"));
					AssertThat(doc.peoples()[2]->name().toStdString(), Equals("moze"));
					AssertThat(doc.peoples()[3]->name().toStdString(), Equals("suzie"));

					AssertThat(doc.texts().count(), Equals(546));
					AssertThat(doc.texts()[0]->people()->name().toStdString(), Equals("ned"));
					AssertThat(doc.texts()[0]->y(), Equals(0.213333338f));
					AssertThat(doc.texts()[0]->height(), Equals(0.28666667f));
					AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:00:13"));
					AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType25), Equals("01:00:01:05"));
				});

				it("import_drb03", [&]() {
					AssertThat(doc.openStripFile("drb03.drb"), IsTrue());

					AssertThat(doc.videoFilePath().toStdString(), Equals("C:\\Users\\Matthhou\\Desktop\\The Crazy Ones\\The Crazy Ones 121\\The_Crazy_Ones_1AXB21_VOVI.mov"));
					AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
					AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("00:58:24:00"));

					AssertThat(doc.loops().count(), Equals(1));

					AssertThat(doc.loops().at(0)->label().toStdString(), Equals("1"));

					AssertThat(t2s(doc.loops()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:00:00"));

					AssertThat(doc.peoples().count(), Equals(2));

					AssertThat(doc.peoples()[0]->name().toStdString(), Equals("Pierre"));
					AssertThat(doc.peoples()[1]->name().toStdString(), Equals("Marie"));

					AssertThat(doc.texts().count(), Equals(2));

					AssertThat(doc.texts()[0]->people()->name().toStdString(), Equals("Pierre"));
					AssertThat(doc.texts()[0]->y(), Equals(0.066666667f));
					AssertThat(doc.texts()[0]->height(), Equals(0.22666667f));
					AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:01:00"));
					AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType25), Equals("01:00:03:00"));

					AssertThat(doc.texts()[1]->people()->name().toStdString(), Equals("Marie"));
					AssertThat(doc.texts()[1]->y(), Equals(0.7f));
					AssertThat(doc.texts()[1]->height(), Equals(0.22666667f));
					AssertThat(t2s(doc.texts()[1]->timeIn(), PhTimeCodeType25), Equals("01:00:04:00"));
					AssertThat(t2s(doc.texts()[1]->timeOut(), PhTimeCodeType25), Equals("01:00:06:00"));

					AssertThat(doc.cuts().count(), Equals(1));
					AssertThat(t2s(doc.cuts()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:05:00"));
				});
			});

			describe("v6", [&]() {
				it("import_test01", [&]() {
					AssertThat(doc.openStripFile("test01.syn6"), IsTrue());

					AssertThat(doc.peoples().count(), Equals(2));
					AssertThat(doc.peoples()[0]->name().toStdString(), Equals("Personnage 1"));
					AssertThat(doc.peoples()[1]->name().toStdString(), Equals("Personnage 2"));

					AssertThat(doc.texts().count(), Equals(1));
					AssertThat(doc.texts()[0]->people()->name().toStdString(), Equals("Personnage 1"));

					AssertThat(doc.texts()[0]->y(), Equals(0.36f));
					AssertThat(doc.texts()[0]->height(), Equals(0.22666667f));

					AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType25), Equals("00:00:03:11"));
					AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType25), Equals("00:00:06:14"));
				});

				it("import_test02", [&]() {
					AssertThat(doc.openStripFile("test02.syn6"), IsTrue());

					AssertThat(doc.videoFilePath().toStdString(), Equals("C:\\Users\\Matthhou\\Desktop\\The Crazy Ones\\The Crazy Ones 121\\The_Crazy_Ones_1AXB21_VOVI.mov"));
					AssertThat(doc.videoTimeCodeType(), Equals(PhTimeCodeType25));
					AssertThat(t2s(doc.videoTimeIn(), PhTimeCodeType25), Equals("00:58:24:00"));

					AssertThat(doc.loops().count(), Equals(1));

					AssertThat(doc.loops().at(0)->label().toStdString(), Equals("1"));

					AssertThat(t2s(doc.loops()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:00:00"));

					AssertThat(doc.peoples().count(), Equals(2));

					AssertThat(doc.peoples()[0]->name().toStdString(), Equals("Pierre"));
					AssertThat(doc.peoples()[1]->name().toStdString(), Equals("Marie"));

					AssertThat(doc.texts().count(), Equals(2));

					AssertThat(doc.texts()[0]->people()->name().toStdString(), Equals("Pierre"));
					AssertThat(doc.texts()[0]->y(), Equals(0.066666667f));
					AssertThat(doc.texts()[0]->height(), Equals(0.22666667f));
					AssertThat(t2s(doc.texts()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:01:00"));
					AssertThat(t2s(doc.texts()[0]->timeOut(), PhTimeCodeType25), Equals("01:00:03:00"));

					AssertThat(doc.texts()[1]->people()->name().toStdString(), Equals("Marie"));
					AssertThat(doc.texts()[1]->y(), Equals(0.7f));
					AssertThat(doc.texts()[1]->height(), Equals(0.22666667f));
					AssertThat(t2s(doc.texts()[1]->timeIn(), PhTimeCodeType25), Equals("01:00:04:00"));
					AssertThat(t2s(doc.texts()[1]->timeOut(), PhTimeCodeType25), Equals("01:00:06:00"));

					AssertThat(doc.cuts().count(), Equals(1));
					AssertThat(t2s(doc.cuts()[0]->timeIn(), PhTimeCodeType25), Equals("01:00:05:00"));
				});
			});

		});

		describe("dubx", [&]() {
			it("open", [&]() {
				AssertThat(doc.openStripFile("test01.dubx"), IsTrue());
			});

			it("import basic dubx file", [&]() {
				AssertThat(doc.importDubXFile("test01.dubx"), IsTrue());

				// Meta
				AssertThat(doc.title().toStdString(), Equals("dubx title"));
				AssertThat(doc.episode().toStdString(), Equals("dubx episode"));
				AssertThat(doc.videoFilePath().toStdString(), Equals("d:\\movie.mov"));
				AssertThat(t2s25(doc.videoTimeIn()), Equals("01:00:00:00"));

				// Peoples
				AssertThat(doc.peoples().count(), Equals(2));

				PhPeople *bob = doc.peoples().at(0);
				AssertThat(bob->name().toStdString(), Equals("Bob"));
				AssertThat(bob->color().toStdString(), Equals("#ff0000"));
				AssertThat(bob->picture().toStdString(), Equals("b64 data "));

				PhPeople *sue = doc.peoples().at(1);
				AssertThat(sue->name().toStdString(), Equals("Sue"));
				AssertThat(sue->color().toStdString(), Equals("#00ff00"));
				AssertThat(sue->picture().toStdString(), Equals("other b64 data "));

				// Loops
				AssertThat(doc.loops().count(), Equals(2));

				PhStripLoop *loop = doc.loops().at(0);
				AssertThat(t2s25(loop->timeIn()), Equals("00:00:00:00"));
				AssertThat(loop->label().toStdString(), Equals("0"));

				loop = doc.loops().at(1);
				AssertThat(t2s25(loop->timeIn()), Equals("01:00:13:20"));
				AssertThat(loop->label().toStdString(), Equals("1"));

				// Texts
				AssertThat(doc.texts().count(), Equals(1));

				PhStripText *text = doc.texts().at(0);

				AssertThat(text->people(), Equals(bob));
				AssertThat(t2s25(text->timeIn()), Equals("01:00:14:14"));
				AssertThat(t2s25(text->timeOut()), Equals("01:00:16:05"));
				AssertThat(text->content().toStdString(), Equals("Une détective privée adolescente"));
				AssertThat(text->y(), Equals(0.5f));
				AssertThat(text->height(), Equals(0.25f));
			});

			it("import other dubx file", [&]() {
				AssertThat(doc.importDubXFile("test02.dubx"), IsTrue());

				// Meta
				AssertThat(doc.title().toStdString(), Equals("test02"));
				AssertThat(doc.episode().toStdString(), Equals(""));
				AssertThat(doc.videoFilePath().toStdString(), Equals("C:\\Users\\Martin\\Dropbox\\DirtyLove\\DirtyLove.mov"));
				AssertThat(t2s25(doc.videoTimeIn()), Equals("10:04:01:11"));

				// Peoples
				AssertThat(doc.peoples().count(), Equals(2));

				PhPeople *cliente = doc.peoples().at(0);
				AssertThat(cliente->name().toStdString(), Equals("Cliente"));
				AssertThat(cliente->color().toStdString(), Equals("#004080"));
				AssertThat(cliente->picture().toStdString(), Equals("/9j/4AAQSkZJRgABAQEAYABgAAD/2wBDAAgGBgcGBQgHBwcJCQgKDBQNDAsLDBkSEw8UHRofHh0aHBwgJC4nICIsIxwcKDcpLDAxNDQ0Hyc5PTgyPC4zNDL/2wBDAQkJCQwLDBgNDRgyIRwhMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjIyMjL/wAARCAB/AH8DASIAAhEBAxEB/8QAHwAAAQUBAQEBAQEAAAAAAAAAAAECAwQFBgcICQoL/8QAtRAAAgEDAwIEAwUFBAQAAAF9AQIDAAQRBRIhMUEGE1FhByJxFDKBkaEII0KxwRVS0fAkM2JyggkKFhcYGRolJicoKSo0NTY3ODk6Q0RFRkdISUpTVFVWV1hZWmNkZWZnaGlqc3R1dnd4eXqDhIWGh4iJipKTlJWWl5iZmqKjpKWmp6ipqrKztLW2t7i5usLDxMXGx8jJytLT1NXW19jZ2uHi4+Tl5ufo6erx8vP09fb3+Pn6/8QAHwEAAwEBAQEBAQEBAQAAAAAAAAECAwQFBgcICQoL/8QAtREAAgECBAQDBAcFBAQAAQJ3AAECAxEEBSExBhJBUQdhcRMiMoEIFEKRobHBCSMzUvAVYnLRChYkNOEl8RcYGRomJygpKjU2Nzg5OkNERUZHSElKU1RVVldYWVpjZGVmZ2hpanN0dXZ3eHl6goOEhYaHiImKkpOUlZaXmJmaoqOkpaanqKmqsrO0tba3uLm6wsPExcbHyMnK0tPU1dbX2Nna4uPk5ebn6Onq8vP09fb3+Pn6/9oADAMBAAIRAxEAPwD3riq11Y214Ua4TcY87D6Zxnjoeg61ZoxxUEHN2vgjSLSSOSMSZjxjMsh6exYj9K3rW1hs7dYLdNkS9BnNTUUXHcQgZo2g/wD66U1Vu7+CyH7xufSgREuk2C3S3CwKJoyWU4GQSCCc9e571PA2d3oKzf8AhI7LcQ2QPXn/AAptnrVqWneSQInG3IJ9farWwixrNnbXkCfaEDGNtyEgHacY7+xNeT6lGsQ3Rj7/ACxAAyeB2+gr1S41SxuECR3AJPbBryrUm3W6n0J/nUS0SBHE3fEp9jWVI7JMrxu0cifddT0z1+taeokC5YVlSnk1vBXREtCQ6vqyW4g/tB/KHt756dK0vCFkl9qksTs4zExY7iS2FPf8K59vr0ro/AzEeIAM43RP/wCgNXTTik9DGpJtanPzIrbVddy49a6aC/v002yKXrjcXB7dD6jGfxrmpDl1rcjX/iU2DdMvJX0CpwmvfVzy+eUfhdj6kooo7V8ce8FFFAoGV7y5W3i3HqelcXqF680haRs+laetXfnXO1T8q8Vy15L859q02QJXEnm4zVGe9CqRuIHsajnl3cDis6aTg5qOYqxae8dTuWVl9DmqU13vgMLjLA5Bqp5uCYz36GoXfclVo9yWjA1M4u2B/wA8VlynmtjVomcrMvPZqw5OvNawVjKRG3Suh8Dc+KIV9Ucf+ONXOEg1v+Byf+EstAOMiQf+Q2rqp7nPPYx5PvLXQRc6HY+0klc8/wB9a6SHH/CP2R/6aP8Azr6GnseXPY+naO1HagdK+MPoAqveT/Z7V5M9BxViua8QXTGVoQflXGaaGYU83zOSawbqT560Z5cLXO31wQ5APPahspCTPyeaoySDvQ0pbOTzWfNdNwBSsXccX/e57CmMcFh6HiolkJOSetOZv3j5qkRJkUrAxyL1yK5i44kOa6Nz+9x2NYWoReVO8f8Ad5H41vAwmUCc10HgZgPGNhn1f/0W1c6e9bngs/8AFZacP9t//QGrpp7mE9jNk/1grfiwfD1rk8iR/wCdc8/3x9TW6hH9gW//AF0b+dfQ0tjy57H1KaO1B6UvavjD3xkjiONn7DrXFalKcHJySea7C7OLZzXG6tH5crITngH86fQa3MC7k2nA9K5u4O5zmt29f5ulYMz4BxUXNLXKsh2q2KzrhPlPbPSrcsrBSNvWoWZWTDVaY7FKAsGTnIyasFvlz61XldYQSB06VIDujwOvUVRDQ2Vv3p9wKzdWQFFfHLcZzVpmywPrUN8DNZEjnYc1rEymjCcYJ5rW8JHHizTuf+Wjf+gmsdxgkHpWr4UfZ4r09j0Eh/8AQTXTTepzVNiiT+9+hNXxLKbCOP8A5ZhiRWef9d+J/nWwmP7Ii4/jNfRUdjzKmx9XHpSUHpS9q+NPeIrhN9u6+orh9SdmcluuMV3Un3DXC6sR9ocD1GKOg47nMXjfMR6Cufun2Nj1rZv5BHcZPQ1iXOS3vULQ0W5Xk3+Xuqj5gkJAGDVmVWwc1V+6D2q00OzK03zsEHVjxVmHG4D04qvCu+UyelSlgkhJHAp3JZBKoDbv4TnFJZbZY2hc43cUsrHdLD3HIqrC+JBng1omZtGVfQvbzyRMOh4p+iTCDW7SVjgK+T+Rqxrzl7vceMj86y4OZ0wcc9a6aW6OaotCyCDP9Sa2IT/xKox6Oax4v9eB9a1rfnT8ejV9LR2PJq6I+rz0o7UGjtXxh74jDKkV59qTbrxgDXfzMVhZh1ArzvUiUkdyOnND2KictqxA29+TWFJLgZatzVASDjsSa5657Ad6hFEMtzweaoTSl+B+NWJIzUIQDJI6datWG2yW2XbAPalwCnPehAQhpjnbtHYZqkSzPndjJHIvcc02fKuso7UjqdijPfrRIWBwOeK0UWzNtLcbqINzYfaB1Tj8zWGrEMDW7C4CSQsPkkGCKw2UpIwPUV0U00znm09i9B/r61Lb/jzYZ/irKtx+9rStT/o7j3FfS0PhPJrH1maB0oNIzbIyx6CvjD3yrfXMcMBRj8zjgVwmovuVwemKu32rST3gdj8qngVmaiSFYDrRPRFROauW+Yhj1GK566OH4ravm2unrk1i3OBKQeahFFRmJ+tQsMAg96lc7GzULHK/jW0Y6EylZ6EyEHoaq3DE5x948AUsb7cHPbmopiVfb1J6GhITehHgOfZaa5DZKilJxx61GScYHeuynCxxVJ3G4GMetUbyAE+Yo571czUb/MpHrW/KZKRDAP3ua0bTmGQe4qhCuH47ir1qTtkH0r3MM7xOGtufWZ6VS1eTy9KnbvtH8xV49KzdbUvpUqjqcY/MV8ctz3zgpFLvikuclGJ6mrzxLbxuzD5u1ZF7K3lbc9airIuKOav5FNxuHIPArMJT7aPMOEHX8qv3OGmJxwtZE5zPn1606S1FU2GagI95MTZUjrWf5mflbg1ZkyCarOu5MGu62hx82pLjCj/aBqGUFcGp0GY1HpVS4k82VsfdAwKxivesbt+7cj5AFJ05NMLEn2FL1HPSu2KOGTIzy2elNJ4HtTmPzcdKa5wMCtEZsamBIRVq36P+FVOjqatwdWr18G7xsc1Y/9k="));

				PhPeople *voyante = doc.peoples().at(1);
				AssertThat(voyante->name().toStdString(), Equals("Voyante"));
				AssertThat(voyante->color().toStdString(), Equals("#800080"));
				AssertThat(voyante->picture().toStdString(), Equals("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"));

				// Loops
				AssertThat(doc.loops().count(), Equals(6));

				PhStripLoop *loop = doc.loops().at(0);
				AssertThat(t2s25(loop->timeIn()), Equals("00:00:00:00"));
				AssertThat(loop->label().toStdString(), Equals("0"));

				loop = doc.loops().at(1);
				AssertThat(t2s25(loop->timeIn()), Equals("10:04:03:23"));
				AssertThat(loop->label().toStdString(), Equals("1"));

				// Texts
				AssertThat(doc.texts().count(), Equals(84));

				PhStripText *text = doc.texts().at(0);

				AssertThat(text->people(), Equals(cliente));
				AssertThat(t2s25(text->timeIn()), Equals("10:04:01:12"));
				AssertThat(t2s25(text->timeOut()), Equals("10:04:05:24"));
				AssertThat(text->content().toStdString(), Equals("(pleure________________________________________________)"));
				AssertThat(text->y(), Equals(0.25f));
				AssertThat(text->height(), Equals(0.25f));

				text = doc.texts().at(83);

				AssertThat(text->people(), Equals(voyante));
				AssertThat(t2s25(text->timeIn()), Equals("10:04:57:01"));
				AssertThat(t2s25(text->timeOut()), Equals("10:04:58:11"));
				AssertThat(text->content().toStdString(), Equals("ride into the sunset"));
				AssertThat(text->y(), Equals(0.0f));
				AssertThat(text->height(), Equals(0.25f));
			});
		});

		it("get people by name", [&](){
			AssertThat(doc.importDetXFile("test01.detx"), IsTrue());

			AssertThat(doc.peopleByName("Jeanne")->name().toStdString(), Equals("Jeanne"));
			AssertThat(doc.peopleByName("Sue")->name().toStdString(), Equals("Sue"));
			AssertThat(doc.peopleByName("Paul")->name().toStdString(), Equals("Paul"));
			AssertThat(doc.peopleByName("Bob") == NULL, IsTrue());
		});

		it("get previous element time", [&](){
			AssertThat(doc.importDetXFile("test01.detx"), IsTrue());
			PhTimeCodeType tcType = PhTimeCodeType25;

			AssertThat(t2s(doc.previousElementTime(s2t("23:00:00:00", tcType)), tcType), Equals("01:01:00:00"));
			AssertThat(t2s(doc.previousElementTime(s2t("01:01:00:00", tcType)), tcType), Equals("01:00:30:00"));
			AssertThat(t2s(doc.previousElementTime(s2t("01:00:30:00", tcType)), tcType), Equals("01:00:15:00"));
			AssertThat(t2s(doc.previousElementTime(s2t("01:00:15:00", tcType)), tcType), Equals("01:00:12:00"));
			AssertThat(doc.previousElementTime(s2t("01:00:00:00", tcType)), Equals(PHTIMEMIN));
		});

		it("get next element time", [&](){
			AssertThat(doc.importDetXFile("test01.detx"), IsTrue());
			PhTimeCodeType tcType = PhTimeCodeType25;

			AssertThat(t2s(doc.nextElementTime(s2t("00:00:00:00", tcType)), tcType), Equals("01:00:00:00"));
			AssertThat(t2s(doc.nextElementTime(s2t("01:00:00:00", tcType)), tcType), Equals("01:00:01:00"));
			AssertThat(t2s(doc.nextElementTime(s2t("01:00:01:00", tcType)), tcType), Equals("01:00:02:00"));
			AssertThat(t2s(doc.nextElementTime(s2t("01:00:02:00", tcType)), tcType), Equals("01:00:05:00"));
			AssertThat(t2s(doc.nextElementTime(s2t("01:00:17:00", tcType)), tcType), Equals("01:00:30:00"));
			AssertThat(t2s(doc.nextElementTime(s2t("01:00:30:00", tcType)), tcType), Equals("01:01:00:00"));
			AssertThat(doc.nextElementTime(s2t("01:01:00:00", tcType)), Equals(PHTIMEMAX));
		});

		it("get next text", [&](){
			AssertThat(doc.importDetXFile("test01.detx"), IsTrue());
			PhTimeCodeType tcType = PhTimeCodeType25;

			AssertThat(t2s(doc.nextText(s2t("00:00:00:00", tcType))->timeIn(), tcType), Equals("01:00:02:00"));
			AssertThat(t2s(doc.nextText(s2t("01:00:02:00", tcType))->timeIn(), tcType), Equals("01:00:05:00"));
			AssertThat(t2s(doc.nextText(s2t("01:00:05:00", tcType))->timeIn(), tcType), Equals("01:00:06:00"));
			AssertThat(t2s(doc.nextText(s2t("01:00:06:00", tcType))->timeIn(), tcType), Equals("01:00:12:00"));
			AssertThat(t2s(doc.nextText(s2t("01:00:12:00", tcType))->timeIn(), tcType), Equals("01:00:15:00"));
			AssertThat(t2s(doc.nextText(s2t("01:00:15:00", tcType))->timeIn(), tcType), Equals("01:00:30:00"));
			AssertThat(doc.nextText(s2t("01:00:30:00", tcType)) == NULL, IsTrue());
		});

		it("get next text by people", [&](){
			AssertThat(doc.importDetXFile("test01.detx"), IsTrue());
			PhTimeCodeType tcType = PhTimeCodeType25;

			PhPeople* sue = doc.peopleByName("Sue");

			AssertThat(t2s(doc.nextText(sue, s2t("00:00:00:00", tcType))->timeIn(), tcType), Equals("01:00:05:00"));
			AssertThat(t2s(doc.nextText(sue, s2t("01:00:05:00", tcType))->timeIn(), tcType), Equals("01:00:06:00"));
			AssertThat(t2s(doc.nextText(sue, s2t("01:00:06:00", tcType))->timeIn(), tcType), Equals("01:00:15:00"));
			AssertThat(t2s(doc.nextText(sue, s2t("01:00:15:00", tcType))->timeIn(), tcType), Equals("01:00:30:00"));
			AssertThat(doc.nextText(sue, s2t("01:00:30:00", tcType)) == NULL, IsTrue());
		});

		it("get next text by people list", [&](){
			AssertThat(doc.importDetXFile("test01.detx"), IsTrue());
			PhTimeCodeType tcType = PhTimeCodeType25;

			QList<PhPeople*> peopleList;
			peopleList.append(doc.peopleByName("Sue"));
			peopleList.append(doc.peopleByName("Paul"));

			AssertThat(t2s(doc.nextText(peopleList, s2t("00:00:00:00", tcType))->timeIn(), tcType), Equals("01:00:05:00"));
			AssertThat(t2s(doc.nextText(peopleList, s2t("01:00:05:00", tcType))->timeIn(), tcType), Equals("01:00:06:00"));
			AssertThat(t2s(doc.nextText(peopleList, s2t("01:00:06:00", tcType))->timeIn(), tcType), Equals("01:00:12:00"));
			AssertThat(t2s(doc.nextText(peopleList, s2t("01:00:12:00", tcType))->timeIn(), tcType), Equals("01:00:15:00"));
			AssertThat(t2s(doc.nextText(peopleList, s2t("01:00:15:00", tcType))->timeIn(), tcType), Equals("01:00:30:00"));
			AssertThat(doc.nextText(peopleList, s2t("01:00:30:00", tcType)) == NULL, IsTrue());
		});

		it("get next loop", [&](){
			AssertThat(doc.importDetXFile("test01.detx"), IsTrue());
			PhTimeCodeType tcType = PhTimeCodeType25;

			AssertThat(t2s(doc.nextLoop(s2t("00:00:00:00", tcType))->timeIn(), tcType), Equals("01:00:00:00"));
			AssertThat(t2s(doc.nextLoop(s2t("01:00:00:00", tcType))->timeIn(), tcType), Equals("01:01:00:00"));
			AssertThat(doc.nextLoop(s2t("01:01:00:00", tcType)) == NULL, IsTrue());
		});

		it("get previous loop", [&](){
			AssertThat(doc.importDetXFile("test01.detx"), IsTrue());
			PhTimeCodeType tcType = PhTimeCodeType25;

			AssertThat(doc.previousLoop(s2t("01:00:00:00", tcType)) == NULL, IsTrue());
			AssertThat(t2s(doc.previousLoop(s2t("01:01:00:00", tcType))->timeIn(), tcType), Equals("01:00:00:00"));
			AssertThat(t2s(doc.previousLoop(s2t("23:00:00:00", tcType))->timeIn(), tcType), Equals("01:01:00:00"));
		});

		it("add object", [&](){
			doc.addPeople(new PhPeople("A people"));

			doc.addObject(new PhStripText(0, doc.peoples().last(), 10000, 1, "Hello", 0.25f));
			AssertThat(doc.texts().count(), Equals(1));
			doc.addObject(new PhStripCut(5400, PhStripCut::CrossFade));
			AssertThat(doc.cuts().count(), Equals(1));
			doc.addObject(new PhStripDetect(PhStripDetect::Aperture, 10000, doc.peoples().last(), 11000, 1));
			AssertThat(doc.detects().count(), Equals(1));

			doc.addObject(new PhStripLoop(22000, "3"));
			AssertThat(doc.loops().count(), Equals(1));

		});

		it("add people", [&](){
			doc.addPeople(new PhPeople("A people"));
			AssertThat(doc.peoples().count(), Equals(1));
			doc.addPeople(new PhPeople("A second people"));
			AssertThat(doc.peoples().count(), Equals(2));
		});
	});
});
