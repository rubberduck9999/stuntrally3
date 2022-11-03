#include "TerrainGame.h"
#include "CameraController.h"
#include "GraphicsSystem.h"
#include "SDL_scancode.h"
#include "OgreLogManager.h"

#include "OgreSceneManager.h"
#include "OgreRoot.h"
#include "OgreCamera.h"
#include "OgreWindow.h"
#include "OgreFrameStats.h"

#include "Terra/Terra.h"
#include "OgreItem.h"
#include "OgreHlms.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsManager.h"
#include "OgreGpuProgramManager.h"
#include "OgreTextureGpuManager.h"
#include "OgrePixelFormatGpuUtils.h"
#include "mathvector.h"
#include "quaternion.h"

#ifdef OGRE_BUILD_COMPONENT_ATMOSPHERE
#    include "OgreAtmosphereNpr.h"
#endif

//  SR
#include "pathmanager.h"
#include "settings.h"
#include "CGame.h"
#include "game.h"
#include "CarModel.h"
#include "CScene.h"

#include <list>
#include <filesystem>


using namespace Demo;
using namespace Ogre;
using namespace std;


namespace Demo
{
	TerrainGame::TerrainGame()
		: TutorialGameState()
		//, mIblQuality( IblHigh )  // par
		//, mIblQuality( MipmapsLowest )
	{
		macroblockWire.mPolygonMode = PM_WIREFRAME;
		SetupVeget();
	}


	//  load settings from default file
	void TerrainGame::LoadDefaultSet(SETTINGS* settings, string setFile)
	{
		settings->Load(PATHMANAGER::GameConfigDir() + "/game-default.cfg");
		settings->Save(setFile);
		//  delete old keys.xml too
		string sKeys = PATHMANAGER::UserConfigDir() + "/keys.xml";
		if (std::filesystem::exists(sKeys))
			std::filesystem::rename(sKeys, PATHMANAGER::UserConfigDir() + "/keys_old.xml");
	}


	//  Init SR game
	//-----------------------------------------------------------------------------------------------------------------------------
	void TerrainGame::Init()
	{

		Ogre::Timer ti;
		setlocale(LC_NUMERIC, "C");

		//  Paths
		PATHMANAGER::Init();
		

		///  Load Settings
		//----------------------------------------------------------------
		pSet = new SETTINGS();
		string setFile = PATHMANAGER::SettingsFile();
		
		if (!PATHMANAGER::FileExists(setFile))
		{
			cerr << "Settings not found - loading defaults." << endl;
			LoadDefaultSet(pSet,setFile);
		}
		pSet->Load(setFile);  // LOAD
		if (pSet->version != SET_VER)  // loaded older, use default
		{
			cerr << "Settings found, but older version - loading defaults." << endl;
			std::filesystem::rename(setFile, PATHMANAGER::UserConfigDir() + "/game_old.cfg");
			LoadDefaultSet(pSet,setFile);
			pSet->Load(setFile);  // LOAD
		}


		//  paths
		LogO(PATHMANAGER::info.str());


		///  Game start
		//----------------------------------------------------------------
		pGame = new GAME(pSet);
		pGame->Start();

		pApp = new App(pSet, pGame);
		pApp->mainApp = this;
		pGame->app = pApp;
		sc = pApp->scn->sc;

		pGame->ReloadSimData();

		//  new game
		pApp->mRoot = mGraphicsSystem->getRoot();
		pApp->mCamera = mGraphicsSystem->getCamera();
		pApp->mSceneMgr = mGraphicsSystem->getSceneManager();
		pApp->mDynamicCubemap = mDynamicCubemap;

		pApp->CreateScene();  /// New
		
	}

	void TerrainGame::Destroy()
	{
		if (pGame)
			pGame->End();
	
		delete pApp;
		delete pGame;
		delete pSet;
	}


	
	//  Create
	//-----------------------------------------------------------------------------------------------------------------------------
	void TerrainGame::createScene01()
	{

		mGraphicsSystem->mWorkspace = setupCompositor();

		SceneManager *sceneManager = mGraphicsSystem->getSceneManager();
		SceneNode *rootNode = sceneManager->getRootSceneNode( SCENE_STATIC );

		LogManager::getSingleton().setLogDetail(LoggingLevel::LL_BOREME);

		LogO("---- createScene");
		Root *root = mGraphicsSystem->getRoot();
		RenderSystem *renderSystem = root->getRenderSystem();
		renderSystem->setMetricsRecordingEnabled( true );


		//  camera  ------------------------------------------------
		// mCameraController = new CameraController( mGraphicsSystem, false );
		mGraphicsSystem->getCamera()->setFarClipDistance( 20000.f );  // par far
		// mGraphicsSystem->getCamera()->setFarClipDistance( pSet->view_distance );  // par far

		camPos = Vector3(10.f, 11.f, 16.f );
		//camPos.y += mTerra->getHeightAt( camPos );
		mGraphicsSystem->getCamera()->setPosition( camPos );
		mGraphicsSystem->getCamera()->lookAt( camPos + Vector3(0.f, -1.6f, -2.f) );
		Vector3 objPos;


		LogO(">>>> Init SR ----");
		Init();
		LogO(">>>> Init SR done ----");

		//  Terrain  ------------------------------------------------
		// CreatePlane();  // fast
		// CreateTerrain();  // 5sec
		// CreateVeget();


		LogO("---- base createScene");

		TutorialGameState::createScene01();
	}


	//  Destroy
	//-----------------------------------------------------------------------------------
	void TerrainGame::destroyScene()
	{
		LogO("---- destroyScene");

		DestroyTerrain();
		DestroyPlane();

		LogO("---- base destroyScene");

		TutorialGameState::destroyScene();

		LogO(">>>> Destroy SR ----");
		Destroy();
		LogO(">>>> Destroy SR done ----");
	}

}

/*  for game.cfg
track = 
Test1-Flat
Test2-Asphalt
Test3-Bumps
Test4-TerrainBig

Test7-FluidsSmall
Test8-Objects
Test12-Snow

TestC6-Temp -
TestC8-Align
TestC9-Jumps -
TestC10-Pace
TestC12-SR
TestC13-Rivers

Jng13-Tropic
Jng20-JungleMaze
For12-HighPeaks

Tox2-AcidLakes
Atm3-Orange
Isl10-Treasure
Wnt2-Wet
Wnt11-Glacier
Wnt17-FrozenGarden

Jng23-FollowValley -
Jng25-CantorJungle
For18-MountCaro
Isl17-AdapterIslands
Mos6-TaraMosses
Isl16-SandyMountain

Des11-PersianCity
Des17-NileCity
Grc8-SlopeCity -

Grc10-BeachCity
Aus10-PreythCity
Grc13-YeleyStunts
Isl19-Shocacosh

Mrs1-Mars
Cry1-CrystalMoon

Sur5-FreshBreeze
Sur1-Surreal
Sur3-Chaotic -

Mud4-Taiga
Can3-Vast
Fin1-Lakes
Atm9-SchwabAutumn 5m!

Vlc4-Spikeland
Vlc5-Sad
Vlc12-LavaPools

*/
