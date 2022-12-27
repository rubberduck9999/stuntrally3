#include "pch.h"
#include "Def_Str.h"
#include "Gui_Def.h"
#include "GuiCom.h"
#include "CScene.h"
#include "settings.h"
#include "CApp.h"
#include "CGui.h"
#include "Road.h"
#include "PaceNotes.h"
#include "Grass.h"
#include "MultiList2.h"
// #include "RenderBoxScene.h"
#include "GraphicsSystem.h"
#include "Terra.h"

#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
//#include <LinearMath/btDefaultMotionState.h>
//#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <MyGUI.h>
#include <OgreParticleEmitter.h>
#include <OgreParticleSystem.h>
#include <OgreOverlay.h>
#include <OgreOverlayElement.h>
using namespace Ogre;


///  🖱️ Mouse
//---------------------------------------------------------------------------------------------------------------
void App::processMouse(double fDT)
{
	//  static vars are smoothed
	static Radian sYaw(0), sPth(0);
	static Vector3 sMove(0,0,0);
	static double time = 0.0;
	time += fDT;
	
	const double ivDT = 0.004;  // const interval
	while (time > ivDT)
	{	time -= ivDT;
	
		Vector3 vInpC(0,0,0),vInp;
		Real fSmooth = (powf(1.0f - pSet->cam_inert, 2.2f) * 40.f + 0.1f) * ivDT;

		const Real sens = 0.13;
		if (bCam())
			vInpC = Vector3(mx, my, 0)*sens;
		vInp = Vector3(mx, my, 0)*sens;  mx = 0;  my = 0;
		vNew += (vInp-vNew) * fSmooth;

		if (mbMiddle){	mTrans.z += vInpC.y * 1.6f;  }  //zoom
		if (mbRight){	mTrans.x += vInpC.x;  mTrans.y -= vInpC.y;  }  //pan
		if (mbLeft){	mRotX -= vInpC.x;  mRotY -= vInpC.y;  }  //rot

		Real cs = pSet->cam_speed;  Degree cr(pSet->cam_speed);
		Real fMove = 100*cs;  //par speed
		Degree fRot = 300*cr, fkRot = 160*cr;

		Radian inYaw = rotMul * ivDT * (fRot* mRotX + fkRot* mRotKX);
		Radian inPth = rotMul * ivDT * (fRot* mRotY + fkRot* mRotKY);
		Vector3 inMove = moveMul * ivDT * (fMove * mTrans);

		sYaw += (inYaw - sYaw) * fSmooth;
		sPth += (inPth - sPth) * fSmooth;
		sMove += (inMove - sMove) * fSmooth;

		mCamera->yaw( sYaw );
		mCamera->pitch( sPth );
		mCamera->moveRelative( sMove );
	}
}


//---------------------------------------------------------------------------------------------------------------
//  💫 frame events
//---------------------------------------------------------------------------------------------------------------
bool App::frameEnded(float dt)
{
	//  show when in gui on generator subtab
	/*if (ovTerPrv)
	if (bGuiFocus && mWndEdit &&
		mWndEdit->getVisible() && mWndTabsEdit->getIndexSelected() == TAB_Terrain &&
		gui->vSubTabsEdit.size() > TAB_Terrain && gui->vSubTabsEdit[TAB_Terrain]->getIndexSelected() == 2)
		ovTerPrv->show();  else  ovTerPrv->hide();*/

	//  track events
	if (eTrkEvent != TE_None)
	{	switch (eTrkEvent)
		{
			case TE_Load:	LoadTrackEv();  break;
			case TE_Save:	SaveTrackEv();  break;
			case TE_Update: UpdateTrackEv();  break;
			default:  break;
		}
		eTrkEvent = TE_None;
	}
	
	///  input
	// mInputWrapper->capture(false);

	//  road pick
	SplineRoad* road = scn->road;
	if (road)
	{
		const MyGUI::IntPoint& mp = MyGUI::InputManager::getInstance().getMousePosition();
		Real mx = Real(mp.left) / mWindow->getWidth(),
			 my = Real(mp.top) / mWindow->getHeight();
		bool setpos = edMode >= ED_Road || !brLockPos,
			hide = !(edMode == ED_Road && bEdit());
		//; road->Pick(mCamera, mx, my,  setpos, edMode == ED_Road, hide);

		//*****************  temp Ter brush pos  ********************
		road->bHitTer = true;
		road->ndHit->setVisible(1);
		road->posHit = Vector3(
			( my-0.5f) * scn->sc->td.fTerWorldSize, 0,
			(-mx+0.5f) * scn->sc->td.fTerWorldSize );
			
		/*Vector3 pos = posHit;
		//if (iChosen == -1)  // for new
		if (!newP.onTer && bAddH)
			pos.y = newP.pos.y;
		ndHit->setPosition(pos);*/
		road->mTerrain->getHeightAt(road->posHit);
		road->ndHit->setPosition(road->posHit);
		// LogO(fToStr(road->posHit.x)+" "+fToStr(road->posHit.y)+" "+fToStr(road->posHit.z));
	}

	EditMouse();  // edit


	///<>  Ter upd	- - -
	static int tu = 0, bu = 0;
	if (tu >= pSet->ter_skip)
	if (bTerUpd)
	{	bTerUpd = false;  tu = 0;
		// if (scn->mTerrainGroup)
		// 	scn->mTerrainGroup->update();
	}	tu++;

	if (bu >= pSet->ter_skip)
	if (bTerUpdBlend)
	{	bTerUpdBlend = false;  bu = 0;
		//if (terrain)
			scn->UpdBlendmap();
	}	bu++;


	///<>  Edit Ter
	TerCircleUpd();
	bool def = false;
	static bool defOld = false;
	float gd = scn->sc->densGrass;
	static float gdOld = scn->sc->densGrass;

	if (scn->terrain && road && bEdit() && road->bHitTer)
	{
		Real s = shift ? 0.25 : ctrl ? 4.0 :1.0;
		switch (edMode)
		{
		case ED_Deform:
			if (mbLeft) {  def = true;  deform(road->posHit, dt, s);  }else
			if (mbRight){  def = true;  deform(road->posHit, dt,-s);  }
			break;
		case ED_Filter:
			if (mbLeft) {  def = true;  filter(road->posHit, dt, s);  }
			break;
		case ED_Smooth:
			if (mbLeft) {  def = true;  smooth(road->posHit, dt);  }
			break;
		case ED_Height:
			if (mbLeft) {  def = true;  height(road->posHit, dt, s);  }
			break;
		default:  break;
		}
	}

	///  ⛰️ Terrain  ----
	if (mTerra && mGraphicsSystem->getRenderWindow()->isVisible() )
	{
		// Force update the shadow map every frame to avoid the feeling we're "cheating" the
		// user in this sample with higher framerates than what he may encounter in many of
		// his possible uses.
		const float lightEpsilon = 0.0001f;  //** 0.0f slow
		mTerra->update( !scn->sun ? -Vector3::UNIT_Y :
			scn->sun->getDerivedDirectionUpdated(), lightEpsilon );
	}

#if 0
if (pSet->bTrees)
{
	///  upd grass
	if (gd != gdOld)
	{
		Real fGrass = pSet->grass * scn->sc->densGrass * 3.0f;
		for (int i=0; i < scn->sc->ciNumGrLay; ++i)
		{
			const SGrassLayer* gr = &scn->sc->grLayersAll[i];
			if (gr->on)
			{
				Forests::GrassLayer *l = gr->grl;
				if (l)
				{	l->setDensity(gr->dens * fGrass);
					scn->grass->reloadGeometry();
				}
			}
		}
	}

	if (!def && defOld)
	{
		scn->UpdGrassDens();
		//if (grd.rnd)
		//	grd.rnd->update();

		for (int i=0; i < scn->sc->ciNumGrLay; ++i)
		{
			const SGrassLayer* gr = &scn->sc->grLayersAll[i];
			if (gr->on)
			{
				Forests::GrassLayer *l = gr->grl;
				if (l)
				{	l->setDensityMap(scn->grassDensRTex, Forests::MapChannel(std::min(3,i)));  // l->chan);
					//l->applyShader();
					scn->grass->reloadGeometry();
				}
			}
		}
	}
	defOld = def;
	gdOld = gd;
}
#endif


	///  🌳🪨 vegetation toggle  upd 🔁 * * *
	if (bTrGrUpd)
	{	bTrGrUpd = false;
		pSet->bTrees = !pSet->bTrees;

		scn->RecreateTrees();
		scn->grass->Destroy();
		if (pSet->bTrees)
			scn->grass->Create();  // 🌿
	}
	
	
	//  🛣️ roads  upd 🔁
	if (!scn->roads.empty())
	{
		SplineRoad* road1 = scn->roads[0];

		for (SplineRoad* road : scn->roads)
		{
			road->bCastShadow = pSet->shadow_type >= Sh_Depth;
			bool fu = road->RebuildRoadInt();
			
			bool full = road == road1 && fu;
			if (full && scn->pace)  // pace, only for 1st
			{
				scn->pace->SetupTer(scn->terrain);
				road->RebuildRoadPace();
				scn->pace->Rebuild(road, scn->sc, pSet->trk_reverse);
			}
		}
	}


	///**  Render Targets update  //; fixme
	if (edMode == ED_PrvCam)
	{
		scn->sc->camPos = mCamera->getPosition();
		scn->sc->camDir = mCamera->getDirection();
		// if (rt[RT_View].tex)
		// 	rt[RT_View].tex->update();
	}else{
		static int ri = 0;
		if (ri >= pSet->mini_skip)
		{	ri = 0;
			// for (int i=0; i < RT_View; ++i)
			// 	if (rt[i].tex)
			// 		rt[i].tex->update();
		}	ri++;
	}
	//LogO(toStr(dt));

	return true;
}


//  💫 Update
//---------------------------------------------------------------------------------------------------------------
void App::update( float dt )
{
	// BaseApp::frameStarted(dt);
	frameRenderingQueued(dt);  //^
	
	UpdFpsText();


	scn->UpdSun();

	static Real time1 = 0.;
	mDTime = dt;
	
	//  inc edit time
	time1 += mDTime;
	if (time1 > 1.)
	{	time1 -= 1.;  ++scn->sc->secEdited;

		if (bGuiFocus)	//  upd ed info txt
			gui->UpdEdInfo();
	}
	
	if (mDTime > 0.1f)  mDTime = 0.1f;  //min 5fps


	//  update input
	mRotX = 0; mRotY = 0;  mRotKX = 0; mRotKY = 0;  mTrans = Vector3::ZERO;

	//  camera Move,Rot
	if (bCam())
	{
		mTrans.x = mKeys[3] - mKeys[2];  // A,D
		mTrans.z = mKeys[1] - mKeys[0];  // W,S
		mTrans.y = mKeys[5] - mKeys[4];  // Q,E
		
		mRotKY = mKeys[6] - mKeys[7];  // ^,v
		mRotKX = mKeys[8] - mKeys[9];  // <,>
	}

	//  speed multiplers
	moveMul = 1;  rotMul = 1;
	if(shift){	moveMul *= 0.2;	 rotMul *= 0.4;	}  // 16 8, 4 3, 0.5 0.5
	if(ctrl){	moveMul *= 4;	 rotMul *= 2.0;	}
	//if(alt)  {	moveMul *= 0.5;	 rotMul *= 0.5;	}
	//const Real s = (shift ? 0.05 : ctrl ? 4.0 :1.0)

	if (imgCur)  //-
	{
		const MyGUI::IntPoint& mp = MyGUI::InputManager::getInstance().getMousePosition();
		imgCur->setPosition(mp);
		imgCur->setVisible(bGuiFocus || !bMoveCam);
	}

	processMouse(mDTime);

	
	///  gui
	gui->GuiUpdate();
	
	
	//  💧 fluids  upd 🔁
	if (bRecreateFluids)
	{	bRecreateFluids = false;

		scn->DestroyFluids();
		scn->CreateFluids();
		UpdFluidBox();
	}

	//  🔥 emitters  upd 🔁
	if (bRecreateEmitters)
	{	bRecreateEmitters = false;

		scn->DestroyEmitters(false);
		if (bParticles)
			scn->CreateEmitters();
		UpdEmtBox();
	}

	
	//--  3d view upd  (is global in window)
	static bool oldVis = false;
	int tab = mWndTabsEdit->getIndexSelected(), st5 = gui->vSubTabsEdit[TAB_Veget]->getIndexSelected();
	bool vis = mWndEdit && mWndEdit->getVisible() && (tab == TAB_Objects || tab == TAB_Veget && st5 == 1);

	if (oldVis != vis)
	{	oldVis = vis;
		gui->viewCanvas->setVisible(vis);
	}
	if (gui->tiViewUpd >= 0.f)
		gui->tiViewUpd += dt;

	if (gui->tiViewUpd > 0.0f)  //par delay 0.1
	{	gui->tiViewUpd = -1.f;

		/*gui->viewBox->clearScene();
		if (!gui->viewMesh.empty() && ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(gui->viewMesh))
		{	gui->viewSc = gui->viewBox->injectObject(gui->viewMesh);
			gui->updVegetInfo();
	}*/	}
	
	
	//  🌧️ Update rain/snow - depends on camera
	scn->UpdateWeather(mCamera, pSet->bWeather ? 0.f : 1.f);

	// update shader time
	// mTimer += evt.timeSinceLastFrame;
	// mFactory->setSharedParameter("windTimer", sh::makeProperty <sh::FloatValue>(new sh::FloatValue(mTimer)));
	// mFactory->setSharedParameter("waterTimer", sh::makeProperty <sh::FloatValue>(new sh::FloatValue(mTimer)));
	
	/*if (ndCar && road)  ///()  grass sphere test
	{
		const Vector3& p = ndCar->getPosition();  Real r = road->vStBoxDim.z/2;  r *= r;
		mFactory->setSharedParameter("posSph0", sh::makeProperty <sh::Vector4>(new sh::Vector4(p.x,p.y,p.z,r)));
		mFactory->setSharedParameter("posSph1", sh::makeProperty <sh::Vector4>(new sh::Vector4(p.x,p.y,p.z,r)));
	}/**/
	
	
	//  pace vis
	if (scn->pace)
		scn->pace->UpdVis(Vector3::ZERO, edMode == ED_PrvCam);

	
	//  upd terrain generator preview
	if (bUpdTerPrv)
	{	bUpdTerPrv = false;
		updateTerPrv();
	}

	
	///  simulate objects
	if (edMode == ED_Objects && objSim /*&& bEdit()*/)
		BltUpdate(dt);
	
	UpdObjNewNode();


	bFirstRenderFrame = false;


	frameEnded(dt);  //^
}
