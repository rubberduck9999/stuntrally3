#include "pch.h"
#include "Def_Str.h"
#include "Gui_Def.h"
#include "GuiCom.h"
#include "CScene.h"
#include "settings.h"
#include "paths.h"
#include "game.h"
#include "CData.h"
#include "TracksXml.h"
// #include "PaceNotes.h"
#include "Road.h"
#include "CGame.h"
#include "CHud.h"
#include "CGui.h"
// #include "SplitScreen.h"
#include "CarModel.h"
#include "GraphView.h"

#include "Slider.h"
#include "MultiList2.h"
#include <OgreCamera.h>
#include <OgreSceneNode.h>
// #include "RenderBoxScene.h"
#include <MyGUI.h>
using namespace std;
using namespace Ogre;
using namespace MyGUI;


///  Gui Events

//   🚗 Car
//---------------------------------------------------------------------
void CGui::chkAbs(WP wp)
{	if (pChall && !pChall->abs)  return;	ChkEv(abs[iTireSet]);	if (pGame)  pGame->ProcessNewSettings();	}
void CGui::chkTcs(WP wp)
{	if (pChall && !pChall->tcs)  return;	ChkEv(tcs[iTireSet]);	if (pGame)  pGame->ProcessNewSettings();	}

void CGui::chkGear(Ck*)
{
	if (pGame)  pGame->ProcessNewSettings();
}

//  reset to same as in default.cfg
void CGui::btnSSSReset(WP)
{
	pSet->sss_effect[0] = 0.574f;  pSet->sss_velfactor[0] = 0.626f;
	pSet->sss_effect[1] = 0.650f;  pSet->sss_velfactor[1] = 0.734f;
	SldUpd_TireSet();  slSSS(0);
}
void CGui::btnSteerReset(WP)
{
	pSet->steer_range[0] = 1.00f;  pSet->steer_sim[0] = 0.71f;
	pSet->steer_range[1] = 0.76f;  pSet->steer_sim[1] = 1.00f;
	SldUpd_TireSet();  slSSS(0);
}

//  sss sliders,  upd graph
void CGui::slSSS(SV* sv)
{
	///  upd sss graph
	float xs = 5.f, yo = 166.f, x2 = 500.f;
	const IntSize& wi = app->mWndOpts->getSize();
	const float sx = wi.width/1248.f, sy = wi.height/935.f;
	xs *= sx;  yo *= sy;  x2 *= sx;

	std::vector<FloatPoint> points,grid;
	float vmax = 300.f;  // kmh
	const int ii = 200;
	int i;
	for (i = 0; i <= ii; ++i)
	{	float v = float(i)/ii * vmax / 3.6f;  // to m/s
		float f = CARCONTROLMAP_LOCAL::GetSSScoeff(v, pSet->sss_velfactor[iTireSet], pSet->sss_effect[iTireSet]);
		points.push_back(FloatPoint(v * xs, yo - yo * f));
	}
	graphSSS->setPoints(points);

	//  grid lines
	const int y1 = yo +10, y2 = -10, x1 = -10;  //outside
	for (i = 0; i < 6; ++i)  // ||
	{	float v = i * 50.f / 3.6f;
		grid.push_back(FloatPoint(v * xs,  i%2==0 ? y1 : y2));
		grid.push_back(FloatPoint(v * xs,  i%2==0 ? y2 : y1));
	}
	grid.push_back(FloatPoint(x2, y1));
	grid.push_back(FloatPoint(x1, y1));
	for (i = 0; i < 4; ++i)  // ==
	{	grid.push_back(FloatPoint(i%2==0 ? x1 : x2,  yo - yo * i * 0.25f));
		grid.push_back(FloatPoint(i%2==0 ? x2 : x1,  yo - yo * i * 0.25f));
	}
	graphSGrid->setPoints(grid);
}

//  gravel/asphalt
void CGui::tabTireSet(Tab, size_t id)
{
	iTireSet = id;
	SldUpd_TireSet();
	bchAbs->setStateSelected(pSet->abs[id]);
	bchTcs->setStateSelected(pSet->tcs[id]);
	slSSS(0);
}

void CGui::SldUpd_TireSet()
{
	int i = iTireSet;
	svSSSEffect.UpdF(&pSet->sss_effect[i]);
	svSSSVelFactor.UpdF(&pSet->sss_velfactor[i]);
	svSteerRangeSurf.UpdF(&pSet->steer_range[i]);
	svSteerRangeSim.UpdF(&pSet->steer_sim[pSet->gui.sim_mode == "easy" ? 0 : 1]);
}

//  player
void CGui::tabPlayer(Tab, size_t id)
{
	iCurCar = id;
	//  update gui for this car (color h,s,v, name, img)
	bool plr = iCurCar < 4;
	if (plr)
	{
		string c = pSet->gui.car[iCurCar];
		for (size_t i=0; i < carList->getItemCount(); ++i)
		if (carList->getItemNameAt(i).substr(7) == c)
		{	carList->setIndexSelected(i);
			listCarChng(carList, i);
	}	}
	carList->setVisible(plr);
	tbPlr[0]->setIndexSelected(id);  tbPlr[1]->setIndexSelected(id);

	UpdCarClrSld(false);  // no car color change
}


//  🎨 car Color
//---------------------------------------------------------------------
//  3. apply new color to car/ghost
void CGui::SetCarClr()
{
	if (!bGI)  return;
	
	int s = app->carModels.size(), i;
	if (iCurCar == 4)  // ghost
	{
		for (i=0; i < s; ++i)
			if (app->carModels[i]->isGhost() && !app->carModels[i]->isGhostTrk())
				app->carModels[i]->ChangeClr();
	}
	else if (iCurCar == 5)  // track's ghost
	{
		for (i=0; i < s; ++i)
			if (app->carModels[i]->isGhostTrk())
				app->carModels[i]->ChangeClr();
	}else
		if (iCurCar < s)  // player
			app->carModels[iCurCar]->ChangeClr();
}

//  2. upd game set color and sliders
void CGui::UpdCarClrSld(bool upd)
{
	SldUpd_CarClr();
	int i = iCurCar;
	
	int c = pSet->car_clr;
	bool ok = c >= 0 && c < data->colors->v.size() && c < imgsCarClr.size();
	pSet->game.clr[i] = pSet->gui.clr[i];  // copy to apply
	UpdCarClrCur();
	if (upd)
		SetCarClr();
	UpdImgClr();
}

void CGui::UpdCarClrCur()
{
	int c = pSet->car_clr;
	bool ok = c >= 0 && c < data->colors->v.size() && c < imgsCarClr.size();
	imgCarClrCur->setVisible(ok);
	if (!ok)  return;

	Img i = imgsCarClr[c];
	auto p = i->getPosition();
	auto s = i->getSize();
	imgCarClrCur->setCoord(p.left-3, p.top-3, s.width+6, s.height+6);
}

//  1. upd sld and pointers after tab change
void CGui::SldUpd_CarClr()
{
	int i = iCurCar;
	svCarClrH.UpdF(&pSet->gui.clr[i].hue);
	svCarClrS.UpdF(&pSet->gui.clr[i].sat);
	svCarClrV.UpdF(&pSet->gui.clr[i].val);
	svCarClrGloss.UpdF(&pSet->gui.clr[i].gloss);
	svCarClrMetal.UpdF(&pSet->gui.clr[i].metal);
	svCarClrRough.UpdF(&pSet->gui.clr[i].rough);
}

void CGui::slCarClr(SV*)
{
	SetCarClr();
	UpdImgClr();
	
	//  upd data for ini save
	int i = iCurCar;  // plr
	int b = pSet->car_clr;  // btn
	if (b >= 0 && b < data->colors->v.size())
		data->colors->v[b] = pSet->gui.clr[i];
}

void CGui::UpdImgClr()
{
	int i = iCurCar;
	float h = pSet->gui.clr[i].hue, s = pSet->gui.clr[i].sat, v = pSet->gui.clr[i].val;
	ColourValue c;  c.setHSB(1.f - h, s, v);
	Colour cc(c.r, c.g, c.b);
	imgCarClr->setColour(cc);
	
	i = pSet->car_clr;  // grid btn
	if (i >= 0 && i < imgsCarClr.size())
		imgsCarClr[i]->setColour(cc);
}

//  🎨 color buttons
//---------------------------------------------------------------------
void CGui::imgBtnCarClr(WP img)
{
	int i = iCurCar;
	auto& c = pSet->car_clr;
	c = s2i(img->getUserString("i"));
	
	if (c >= data->colors->v.size())  return;
	pSet->gui.clr[i] = data->colors->v[c];
	UpdCarClrSld();
}
void CGui::btnCarClrRandom(WP)
{
	int i = iCurCar;
	pSet->car_clr = -1;  //-
	pSet->gui.clr[i].hue = Math::UnitRandom();
	pSet->gui.clr[i].sat = Math::UnitRandom();
	pSet->gui.clr[i].val = Math::UnitRandom();
	pSet->gui.clr[i].gloss = Math::UnitRandom();
	pSet->gui.clr[i].metal = Math::RangeRandom(0.f,1.f);
	pSet->gui.clr[i].rough = Math::RangeRandom(0.01f,0.5f);
	UpdCarClrSld();
}

void CGui::btnCarClrSave(WP)
{
	auto user = PATHS::UserConfigDir() + "/colors.ini";
	data->colors->SaveIni(user);
}
void CGui::btnCarClrLoadDef(WP)
{
	data->LoadColors(true);
	UpdCarClrImgs();
}
void CGui::btnCarClrLoad(WP)
{
	data->LoadColors();
	UpdCarClrImgs();
}

//  clr del -
void CGui::btnCarClrDel(WP)
{
	auto& i = pSet->car_clr;
	auto& v = data->colors->v;
	if (i >= 0 && i < v.size())
		v.erase(v.begin() + i);
	if (i == v.size() && i > 0)
		--i;
	UpdCarClrImgs();
}

//  🎨 clr add +
void CGui::btnCarClrAdd(WP)
{
	CarColor c = pSet->gui.clr[iCurCar];
	auto i = pSet->car_clr;
	auto& v = data->colors->v;
	if (i >= 0 && i < v.size()-1)
		v.insert(v.begin() + i + 1, c);
	else
		v.push_back(c);
	// insert after car_clr?
	UpdCarClrImgs();
}

void CGui::UpdCarClrImgs()
{
	const int clrBtn = data->colors->v.size(),
		clrRow = data->colors->perRow, sx = data->colors->imgSize;
	
	for (auto img : imgsCarClr)
		tbCarClr->_destroyChildWidget(img);
	imgsCarClr.clear();
	
	for (int i=0; i < clrBtn; ++i)
	{
		int x = i % clrRow, y = i / clrRow;
		Img img = tbCarClr->createWidget<ImageBox>("ImageBox",
			12+x*sx, 102+y*sx, sx-1,sx-1, Align::Left, "carClr"+toStr(i));
		img->setImageTexture("white.png");
		gcom->setOrigPos(img, "GameWnd");

		const CarColor& cl = data->colors->v[i];
		float h = cl.hue, s = cl.sat, v = cl.val;
		
		Ogre::ColourValue c;  c.setHSB(1.f-h, s, v);
		img->setColour(Colour(c.r,c.g,c.b));
		img->eventMouseButtonClick += newDelegate(this, &CGui::imgBtnCarClr);

		img->setUserString("i", toStr(i));
		imgsCarClr.push_back(img);
	}
	if (bGI)  // resize
		gcom->doSizeGUI(tbCarClr->getEnumerator());
	UpdCarClrCur();
}


//  💨🔨 Game
//---------------------------------------------------------------------

void CGui::comboBoost(CMB)
{
	pSet->gui.boost_type = val;  app->hud->Show();
}
void CGui::comboFlip(CMB)
{
	pSet->gui.flip_type = val;
}
void CGui::comboDamage(CMB)
{
	pSet->gui.damage_type = val;
}
void CGui::comboRewind(CMB)
{
	pSet->gui.rewind_type = val;
}
	
void CGui::radUpd(bool kmh)
{
	Radio2(bRkmh, bRmph, kmh);
	pSet->show_mph = !kmh;  hud->Size();
	// if (app->scn->pace)
	// 	app->scn->pace->UpdTxt();
}
void CGui::radKmh(WP wp){	radUpd(true);   }
void CGui::radMph(WP wp){	radUpd(false);  }


void CGui::btnNumPlayers(WP wp)
{
	auto& plr = pSet->gui.local_players;
	if (wp)
	{	if      (wp->getName() == "btnPlayers1")  plr = 1;
		else if (wp->getName() == "btnPlayers2")  plr = 2;
		else if (wp->getName() == "btnPlayers3")  plr = 3;
		else if (wp->getName() == "btnPlayers4")  plr = 4;
	}
	if (valLocPlayers)
		valLocPlayers->setCaption(toStr(plr));

	for (int t = 0; t < 2; ++t)  // hide tabs
	for (int p = 1; p < (t == 0 ? 6 : 4); ++p)
		tbPlr[t]->setButtonWidthAt(p, plr > p ? -1 : 1);
}

void CGui::chkStartOrd(WP wp)
{
	pSet->gui.start_order = pSet->gui.start_order==0 ? 1 : 0;
	Btn chk = wp->castType<Button>();
	chk->setStateSelected(pSet->gui.start_order > 0);
}


//  📊 Graphics  options  (game only)
//---------------------------------------------------------------------

//  reflection
void CGui::slReflDist(SV*)
{
	// app->recreateReflections();
}
void CGui::slReflMode(SV* sv)
{
	if (bGI)
	switch (pSet->refl_mode)
	{
		case 0: sv->setTextClr(0.0, 1.0, 0.0);  break;
		case 1: sv->setTextClr(1.0, 0.5, 0.0);  break;
		case 2: sv->setTextClr(1.0, 0.0, 0.0);  break;
	}
	// app->recreateReflections();
}


void CGui::chkParTrl(Ck*)
{		
	for (auto* car : app->carModels)
		car->UpdParsTrails();
}


//  [View] size  . . . . . . . . . . . . . . . . . . . .
void CGui::slHudSize(SV*)
{
	hud->Size();
}
void CGui::slHudCreate(SV*)
{
	hud->Destroy();  hud->Create();
}
void CGui::chkHudCreate(Ck*)
{
	hud->Destroy();  hud->Create();
}

void CGui::slSizeArrow(SV*)
{
	float v = pSet->size_arrow * 0.5f;
	if (hud->arrow.nodeRot)
		hud->arrow.nodeRot->setScale(v * Vector3::UNIT_SCALE);
}
void CGui::slCountdownTime(SL)
{
	float v = (int)(val * 10.f +slHalf) * 0.5f;	if (bGI)  pSet->gui.pre_time = v;
	if (valCountdownTime){	valCountdownTime->setCaption(fToStr(v,1,4));  }
}


//  🌍 View  . . . . . . . . . . . . . . . . . . . .    ---- checks ----    . . . . . . . . . . . . . . . . . . . .

void CGui::chkWireframe(Ck*)
{
	app->SetWireframe();
	// if (app->ndSky)
	// 	app->ndSky->setVisible(!b);  // hide sky-
}

//  ⏱️ HUD
void CGui::chkHudShow(Ck*)
{
	hud->Show();
}

void CGui::chkArrow(Ck*)
{
	if (hud->arrow.nodeRot)
		hud->arrow.nodeRot->setVisible(pSet->check_arrow && !app->bHideHudArr);
}
void CGui::chkBeam(Ck*)
{
	for (int i=0; i < app->carModels.size(); ++i)
		app->carModels[i]->ShowNextChk(pSet->check_beam && !app->bHideHudBeam);
}

//  hud minimap
void CGui::chkMinimap(Ck*)
{
	for (int c=0; c < hud->hud.size(); ++c)
	{	if (hud->hud[c].ndMap)
			hud->hud[c].ndMap->setVisible(pSet->trackmap);
		if (hud->ndPos)
			hud->ndPos->setVisible(pSet->trackmap);
	}
}

void CGui::chkMiniUpd(Ck*)
{
	hud->UpdMiniTer();
}

//  🚦 pacenotes
void CGui::slUpd_Pace(SV*)
{
	// app->scn->UpdPaceParams();
}

//  🎗️ trail
void CGui::chkTrailShow(Ck*)
{
	if (!app->scn->trail)  return;
	if (!pSet->trail_show)
		app->scn->trail->SetVisTrail(false);
}

void CGui::chkReverse(Ck*) {  gcom->ReadTrkStats();  }


//  📉 Graphs  -^-.-

void CGui::chkGraphs(Ck*)
{
	bool te = pSet->graphs_type == Gh_TireEdit;
	for (int i=0; i < app->graphs.size(); ++i)
		app->graphs[i]->SetVisible(!te ? pSet->show_graphs :  // reference vis
			pSet->show_graphs && (i < 2*App::TireNG || i >= 4*App::TireNG || pSet->te_reference));
}
void CGui::comboGraphs(CMB)
{
	if (valGraphsType)
		valGraphsType->setCaption(toStr(val));
	if (bGI /*&& pSet->graphs_type != v*/)  {
		pSet->graphs_type = (eGraphType)val;
		app->DestroyGraphs();  app->CreateGraphs();  }
}


//  [Effects]  . . . . . . . . . . . . . . . . . . . .    ---- ------ ----    . . . . . . . . . . . . . . . . . . . .

/*void CGui::chkAllEffects(Ck*)
{
	app->recreateCompositor();  //app->refreshCompositor();
	app->scn->changeShadows();
}
void CGui::chkEffUpd(Ck*)
{		
	app->refreshCompositor();
}
void CGui::chkEffUpdShd(Ck*)
{
	app->refreshCompositor();
	app->scn->changeShadows();
}

void CGui::slEffUpd(SV*)
{
	if (bGI)  app->refreshCompositor();
}*/


//  🔊 Sound
void CGui::slVolMaster(SV*)
{
	pGame->ProcessNewSettings();
}

void CGui::slVolHud(SV*)
{
	pGame->UpdHudSndVol();
}


//  👈 Hints  How to play
//---------------------------------------------------------------------
const int CGui::iHints = 28;
const static char hintsOrder[CGui::iHints] = {
/*	0 Rewind  22 Keyboard  16 Camera
	1 Turns  3 Gravel  26 Test tracks
	17 Trail  18 Pacenotes  5 Ghost cars
	2 Flipping  4 Damage  6 Boost
	9 Jumps  19 Stunt Pacenotes  10 Loops  11 Pipes
	20 Fluids  27 Handbrake
	7 Cars  12 Vehicles  21 Wheels
	13 Simulation  8 Steering  14 Fps
	23 Editors  24 Details
	15 Last Help  25 Support  */
	0,22,16, 1,3,26, 17,18,5, 2,4,6,
	9,19,10,11, 20,27, 7,12,21,
	13,8,14, 23,24, 15,25
};

//  Lesson replay start  >> >  --------
void CGui::btnLesson(WP wp)
{
	string s = wp->getName(), file;
	int n = s[s.length() - 1] - '0';

	rplSubtitles.clear();
	auto Add = [&](float b, float e, int i)
	{
		string s = "#{Hint-"+toStr(i)+"text}";
		rplSubtitles.push_back(Subtitle(b, e, i, s));
	};

	switch (n)  // hints not here: 5!gho,8, 13-15, 23,25-26
	{	//  Subtitles  ----
	case 1:  file = "1";  //  keys  turns trail  gravel pace
		Add(1.f,  8.f,  22);
		Add(10.f, 21.f, 1 );
		Add(23.f, 30.f, 17);
		Add(32.f, 44.f, 3 );
		Add(46.f, 62.f, 18);
		break;
	case 2:  file = "2";  //  damage  flip  rewind
		Add(0.1f, 11.f, 4 );
		Add(12.f, 18.f, 2 );
		Add(19.f, 33.f, 0 );
		break;
	case 3:  file = "3";  //  boost  jumps
		Add(0.5f,  8.f, 6 );
		Add( 9.f, 18.f, 9 );
		Add(19.f, 38.f, 19);
		break;
	case 4:  file = "4b"; //  pipes  loops
		Add(0.5f, 9.f,  11);
		Add(10.f, 21.f, 10);
		break;
	case 5:  file = "5b"; //  fluids  camera  cars
		Add(0.5f, 21.f, 20);
		Add(22.f, 31.f, 16);
		Add(32.f, 46.f, 7 );
		break;
	case 6:  file = "6";  //  test  handbrake  wheels
		Add(0.5f, 14.f, 26);
		Add(15.f, 24.f, 27);
		Add(25.f, 40.f, 21);
		break;
	case 7:  file = "7";  //  vehicles  details
		Add(1.f,  15.f, 12);
		Add(16.f, 29.f, 24);
		break;
	}
	file = PATHS::Lessons() + "/" + file + ".rpl";
	bLesson = true;  //`
	btnRplLoadFile(file);
	pSet->game.track_reversed = file.find('b') != string::npos;  //app->replay.header.reverse;
	
	rplSubText->setCaption("");  app->mWndRplTxt->setVisible(false);
	//  hud setup, restore ..
	// ckTrailShow.SetValue(1);  /*ckPaceShow.SetValue(1);*/  ckMinimap.SetValue(1);
	ckTimes.SetValue(0);
}

//---------------------------------------------------------------------
const static short hintsImg[CGui::iHints][5] = {
	{0, 4*128, 3*128, 128,128}, //  0 rewind <<
	{0, 6*128, 5*128, 128,128}, //  1 turn -
	{0, 4*128, 1*128, 128,128}, //  2 flip ^
	{0, 3*128, 4*128, 128,128}, //  3 gravel
	{0, 5*128, 3*128, 128,128}, //  4 damage
	{0, 4*128, 5*128, 128,128}, //  5 ghost --
	{0, 4*128, 0*128, 128,128}, //  6 boost
	{0, 1*128, 2*128, 128,128}, //  7 cars
	{0, 1*128, 3*128, 128,128}, //  8 steer

	{1, 192, 0, 32, 32},        //  9 Jumps
	{1, 224, 0, 32, 32},        // 10 Loops
	{1, 256, 0, 32, 32},        // 11 Pipes

	{0, 4*128, 5*128, 128,128}, // 12 vehicles --
	{1,   0, 0, 32, 32},        // 13 sim
	{0, 1*128, 7*128, 128,128}, // 14 fps
	{0, 0*128, 5*128, 128,128}, // 15 help
	{0, 0*128, 4*128, 128,128}, // 16 camera

	{0, 6*128, 5*128, 128,128}, // 17 trail --
	{0, 5*128, 4*128, 128,128}, // 18 pace
	{1, 192, 0, 32, 32},        // 19 pace jmp vel

	{1, 128, 0, 32, 32},        // 20 fluids ~
	{0, 3*128, 5*128, 128,128}, // 21 wheels
	{2, 837, 204, 187,187},     // 22 keys

	{0, 0*128, 7*128, 128,128}, // 23 editors
	{0, 5*128, 7*128, 128,128}, // 24 details
	{0, 5*128, 5*128, 128,128}, // 25 support
	{0, 4*128, 4*128, 128,128}, // 26 test trks -
	{0, 6*128, 4*128, 128,128}, // 27 handbrake
};
const static char* hintTex[3] = {
	"gui_icons.png", "track_icons.png", "keys.png" };

//  upd hint text, img  ----
void CGui::UpdHint()
{
	if (!edHintTitle)  return;
	int h = hintsOrder[iHintCur];
	
	edHintTitle->setCaption(TR("#C0E0FF#{Hint}  #A0D0FF") +toStr(iHintCur+1)+"/"+toStr(iHints)+
					  ":   "+TR("#D0E8FF#{Hint-"+toStr(h)+"}"));
	edHintText->setCaption(TR("#{Hint-"+toStr(h)+"text}"));
	setHintImg(imgHint, h);
}

void CGui::setHintImg(Img img, int h)
{
	auto i = hintsImg[h];
	img->setImageInfo(hintTex[i[0]], IntCoord(i[1],i[2],i[3],i[4]), IntSize(i[3],i[4]));
}


void CGui::btnHintPrev(WP)
{
	iHintCur = (iHintCur-1+iHints) % iHints;  UpdHint();
}
void CGui::btnHintNext(WP)
{
	iHintCur = (iHintCur+1) % iHints;         UpdHint();
}

void CGui::btnHintScreen(WP)
{
	GuiShortcut(MN_Options, TABo_Screen,0);  btnHintClose(0);
}
void CGui::btnHintInput(WP)
{
	GuiShortcut(MN_Options, TABo_Input,0);  btnHintClose(0);
}

void CGui::btnHintClose(WP)
{
	app->mWndWelcome->setVisible(false);
}

void CGui::btnHowToBack(WP)
{
	pSet->iMenu = MN1_Race;  toggleGui(false);
}


///  3d car view  // todo: ..
//---------------------------------------------------------------------
IntCoord CGui::GetViewSize()
{
	IntCoord ic = app->mWndGame->getClientCoord();
	return IntCoord(ic.width*0.56f, ic.height*0.38f, ic.width*0.43f, ic.height*0.57f);
}

void CGui::InitCarPrv()
{
#if 0
	viewCanvas = app->mWndGame->createWidget<Canvas>("Canvas", GetViewSize(), Align::Stretch);
	viewCanvas->setInheritsAlpha(false);
	viewCanvas->setPointer("hand");
	viewCanvas->setVisible(true);
	viewBox->setCanvas(viewCanvas);
	viewBox->setBackgroundColour(Colour(0.32,0.35,0.37,1));
	//viewBox->setAutoRotation(true);
	viewBox->setMouseRotation(true);

	//viewBox->injectObject("sphere.mesh");
	viewCar = new CarModel(3, 0, CarModel::CT_GHOST, "XZ", viewBox->mScene, pSet, pGame, app->scn->sc, 0, app);
	viewCar->Load(-1, false);
	viewCar->Create();
	viewCar->ChangeClr();

	PosInfo p;  p.bNew = true;
	p.pos = Vector3(0,0,0);
	p.rot = Quaternion(Degree(180),Vector3(1,0,0)) * Quaternion(Degree(50),Vector3(0,1,0));
	p.whPos[0] = Vector3(0,-1,-1);  p.whRot[0] = p.rot;
	
	viewCar->Update(p, p, 0.f);
	viewBox->mCamera->setPosition(Vector3(0,4,-7));
	viewBox->mCamera->setDirection(-Vector3(0,4,-7));
	//viewBox->mCameraNode->setPosition(Vector3(0,2,4));
	//viewBox->mCameraNode->lookAt(Vector3(0,0,0), Node::TS_WORLD);
	//viewBox->updateViewport();
#endif
}
