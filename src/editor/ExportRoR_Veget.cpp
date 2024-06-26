#include "pch.h"
#include "ExportRoR.h"
#include "Def_Str.h"
#include "paths.h"

#include "CApp.h"
#include "CGui.h"
#include "CScene.h"
#include "CData.h"
#include "PresetsXml.h"
#include "Axes.h"
#include "Road.h"
#include "TracksXml.h"

#include <Terra.h>
#include <OgreString.h>
#include <OgreImage2.h>
#include <OgreVector3.h>
#include <OgreColourValue.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <map>
using namespace Ogre;
using namespace std;


//------------------------------------------------------------------------------------------------------------------------
//  🌳🪨 Vegetation setup  save  .tobj .png
//------------------------------------------------------------------------------------------------------------------------
void ExportRoR::ExportVeget()
{

	const Real tws = sc->tds[0].fTerWorldSize;
	const auto* terrain = scn->ters[0];

	hasVeget = 1;  // par.. not in FPS variant
	if (hasVeget)
	{
		string vegFile = path + name + "-veget.tobj";
		ofstream veg;
		veg.open(vegFile.c_str(), std::ios_base::out);

		ofstream mat;
		//- if (hasGrass)
		if (copyVeget)
		{
			string matFile = path + name + "-veget.material";
			mat.open(matFile.c_str(), std::ios_base::out);
		}
		
		//  read road dens, once, add outline for trees
		//------------------------------------------------------------
		Ogre::Timer tiRd;
		char* rdGr = 0;   // [] roadDens copy  grass
		char* rdTr = 0;  //  for trees, thick, outline
		
		Image2 img;  PixelFormatGpu pf;
		TextureBox tb;
		int xx = 0, yy = 0;
		try
		{
			img.load(String("roadDensity.png"), "General");
			xx = img.getWidth();  yy = img.getHeight();
			pf = img.getPixelFormat();
			tb = img.getData(0);

			// im2.createEmptyImage(xx, yy, 1, TextureTypes::Type2D, pf);
			rdGr = new char[xx * yy];
			rdTr = new char[xx * yy];
		}
		catch (exception ex)
		{
			gui->Exp(CGui::WARN, string("Exception in road dens map: ") + ex.what());
		}						
		if (!rdGr || !rdTr)
			return;

		// TextureBox tb = img.getData(0);
		// TextureBox tb2 = im2.getData(0);

		int a = 0;
		for (int y = 0; y < yy; ++y)
		for (int x = 0; x < xx; ++x, ++a)
		{
			float rd = tb.getColourAt(xx-1 - y, x, 0, pf).r;  // flip x
			rdGr[a] = rd > 0.995f ? 0 : 1;
		}

		const int d = cfg->roadVegetDist, dd = d + 3;  // par ..
		// const int d = 5, dd = d;
		// const int d = 1, dd = d;  // fast
		for (int y = d; y < yy-d; ++y)
		for (int x = d; x < xx-d; ++x)
		{
			char b = 0;
			for (int jj = -d; jj <= d; ++jj)
			for (int ii = -d; ii <= d; ++ii)
			if (ii + jj < dd)
			{
				char c = rdGr[ (y+jj) * xx + x+ii ];
				if (c > b)  b = c;
				rdTr[y*xx + x] = b;

				// ColourValue cv{b ? 1.f : 0.f, 0.f, 0.f};
				// tb2.setColourAt(cv, x, y, 0, pf);
			}
		}
		// im2.save(path + name + "-ROAD a.png", 0, 0);

		gui->Exp(CGui::INFO, "Veget road density Time: " + fToStr(tiRd.getMilliseconds()/1000.f,1,3) + " s\n");
		Ogre::Timer ti;
	// return;  // test
	

		//------------------------------------------------------------------------------------------------------------------------
		//  🌿 Grass layers
		//------------------------------------------------------------------------------------------------------------------------
		veg << "// grass range,  SwaySpeed, SwayLength, SwayDistribution,   Density,  minx, miny, maxx, maxy,   fadetype, minY, maxY,   material colormap densitymap\n";

		const SGrassLayer* g0 = &sc->grLayersAll[0];
		int igrs = 0;
		for (int i=0; i < sc->ciNumGrLay; ++i)
		{
			const SGrassLayer* gr = &sc->grLayersAll[i];
			if (gr->on)
			{
				const SGrassChannel* ch = &scn->sc->grChan[gr->iChan];
				const string mapName = name + "-grass"+toStr(gr->iChan)+".png";
				++igrs;

				//grass 200,  0.5, 0.05, 10,  0.1, 0.2, 0.2, 1, 1,  1, 0, 9, seaweed none none
				//grass 600,  0.5, 0.15, 10,  0.3, 0.2, 0.2, 1, 1,  1, 10, 0, grass1 aspen.jpg aspen_grass_density.png

				//  write  ------
				//  range,
				veg << "grass " << "300,  ";  // par mul
				//  Sway: Speed, Length, Distribution,
				veg << "0.5, 0.3, 10,  ";  
				
				//  Density,  minx, miny, maxx, maxy,
				veg << gr->dens * sc->densGrass * cfg->grassMul << ",  ";
				veg << gr->minSx <<", "<< gr->minSy <<", "<< gr->maxSx <<", "<< gr->maxSy << ", ";
				
				//  fadetype, minY, maxY,
				veg << "2, 0, 0,  ";
				//  material  colormap  densitymap
				//+  prefix grasses sr-
				veg << "sr-" << gr->material << " none " << mapName << "\n";


				//  grass dens map
				//------------------------------------------------------------
				Image2 im2;
				// try
				// {
					// im2.load(String("roadDensity.png"), "General");
					im2.createEmptyImage(xx, yy, 1, TextureTypes::Type2D, pf);

					TextureBox tb2 = im2.getData(0);
					auto pf = img.getPixelFormat();

					int a = 0;
					for (int y = 0; y < yy; ++y)
					for (int x = 0; x < xx; ++x,++a)
					{
						float rd = 1 - rdGr[a];
							//tb.getColourAt(xx-1 - y, x, 0, pf).r;  // flip x
						
						float xw = (float(x) / (xx-1) - 0.5f) * tws,
							  zw = (float(yy-1 - y) / (yy-1) - 0.5f) * tws;

						Real a = terrain->getAngle(xw, zw, 1.f);
						Real h = terrain->getHeight(xw, zw);  // /2 par..
						float d = rd *
							scn->linRange(a, ch->angMin, ch->angMax, ch->angSm) *
							scn->linRange(h, ch->hMin, ch->hMax, ch->hSm);

						ColourValue cv(d,d,d);  // white
						tb2.setColourAt(cv, y, x, 0, pf);
					}
					im2.save(path + mapName, 0, 0);
				// }
				// catch (exception ex)
				// {
				// 	gui->Exp(CGui::WARN, string("Exception in grass dens map: ") + ex.what());
				// }

				if (copyVeget)  // NO, once for all
				{
					//  copy grass*.png
					//------------------------------------------------------------
					String pathGrs = PATHS::Data() + "/grass/";
					String grassPng = gr->material + ".png";  // same as mtr name
					
					//  copy _d _n
					string from = pathGrs + grassPng, to = path + grassPng;
					CopyFile(from, to);
					
					//  create .material for it
					mat << "material " << gr->material << "\n";
					mat << "{\n";
					mat << "	technique\n";
					mat << "	{\n";
					mat << "		pass\n";
					mat << "		{\n";
					mat << "			cull_hardware none\n";
					mat << "			cull_software none\n";
					mat << "			alpha_rejection greater 128\n";
					mat << "			texture_unit\n";
					mat << "			{\n";
					mat << "				texture	" << grassPng << "\n";
					mat << "				tex_address_mode clamp\n";
					mat << "			}\n";
					mat << "		}\n";
					mat << "	}\n";
					mat << "}\n";
					mat << "\n";
				}
			}
		}
		mat.close();

		gui->Exp(CGui::NOTE, "Grasses: " + toStr(igrs));



		//------------------------------------------------------------------------------------------------------------------------
		//  🌳🪨 Vegetation  layers
		//------------------------------------------------------------------------------------------------------------------------
		std::vector<string> dirs{"trees", "trees2", "trees-old", "rocks", "rockshex"};
		std::map<string, int> once;
		int iVegetMesh = 0;

		for (size_t l=0; l < sc->vegLayers.size(); ++l)
		{
			VegetLayer& vg = sc->vegLayersAll[sc->vegLayers[l]];
			if (!vg.on)
				continue;
			String mesh = vg.name;
			const string mapName = name + "-veget"+toStr(l)+".png";

			//  veget dens map
			//------------------------------------------------------------
			Image2 im2;
			// try
			// {
				im2.createEmptyImage(xx, yy, 1, TextureTypes::Type2D, pf);
				TextureBox tb2 = im2.getData(0);

				int a = 0;
				for (int y = 0; y < yy; ++y)
				for (int x = 0; x < xx; ++x,++a)
				{
					float rd = 1 - rdTr[a];

					float xw = (float(x) / (xx-1) - 0.5f) * tws,
						  zw = (float(yy-1 - y) / (yy-1) - 0.5f) * tws;

					Real a = terrain->getAngle(xw, zw, 1.f);  //td.fTriangleSize);
					Real h = terrain->getHeight(xw, zw);
					float d = 1.f;
					if (a > vg.maxTerAng || rd == 0.f ||
						h > vg.maxTerH || h < vg.minTerH)
						d = 0.f;
					else
					{	//  check if in fluids
						float fa = sc->GetDepthInFluids(Vector3(xw, h, zw));
						if (fa > vg.maxDepth)
							d = 0.f;
						else
						{
						#if 0  //  terribly slow ..
						//------------------------------------------------------------
							int c = sc->trRdDist + vg.addRdist;
							int d = c;
							bool bMax = vg.maxRdist < 20; //100 slow-
							if (bMax)
								d = c + vg.maxRdist+1;  // not less than c

							//  find dist to road
							int ii,jj, rr, rmin = 3000;  //d
							for (jj = -d; jj <= d; jj+=2)
							for (ii = -d; ii <= d; ii+=2)
							// for (jj = -d; jj <= d; ++jj)
							// for (ii = -d; ii <= d; ++ii)
							{
								const int
									xi = std::max(0,std::min(xx-1, y+ii)),
									yj = std::max(0,std::min(yy-1, x+jj));

								const float cr = tb.getColourAt(
									xi, yj, 0, Ogre::PFG_RGBA8_UNORM_SRGB ).r;
							
								// float rd = 1 - rdTr[(y+jj)*xx + x+ii];  //..
								if (cr < 0.75f)  // par-
								{
									rr = abs(ii)+abs(jj);
									rmin = std::min(rmin, rr);
								}
							}
							if (rmin <= c)
								d = 0.f;

							if (bMax && /*d > 1 &&*/ rmin > d-1)  // max dist (optional)
								d = 0.f;
						#endif
					}	}

					//  write pixel  ------
					//  place tree with 1.f,  0.f none
					float dens = vg.dens * sc->densTrees * cfg->treesMul;  // dens mul
					if (Math::UnitRandom() > dens)  // par
						d = 0.f;
					
					d *= rd;
					ColourValue cv(d,d,d);  // white
					tb2.setColourAt(cv, y, x, 0, pf);  // flip x-y
				}
				im2.save(path + mapName, 0, 0);
			// }
			// catch (exception ex)
			// {
			// 	gui->Exp(CGui::WARN, string("Exception in grass dens map: ") + ex.what());
			// }

			
			//  presets.xml needed
			auto nomesh = mesh.substr(0, mesh.length()-5);
			const PVeget* pveg = scn->data->pre->GetVeget(nomesh);
			if (!pveg)  continue;


			//------------------------------------------------------------
			//  Find mesh  in old SR dirs
			//------------------------------------------------------------
			bool exists = 0;
			string from, to;
			
			for (auto& d : dirs)
			{
				string file = pSet->pathExportOldSR + d +"/"+ mesh;
				if (PATHS::FileExists(file))
				{	exists = 1;  from = file;  }
				// gui->Exp(CGui::NOTE, String("veget: ")+file+ "  ex: "+(exists?"y":"n"));
			}
			if (!exists)  //! skip  RoR would crash
			{
				gui->Exp(CGui::WARN, "veget not found in old SR: "+mesh);
				continue;
			}
			RenameMesh(mesh);
			if (!AddPackFor(mesh))  //+
				continue;
			

			//  copy mesh from old SR
			//----------------------------------
			if (exists)
			{
				string name = mesh;

				if (!copyVeget)
					++iVegetMesh;
				else
				if (once.find(mesh) == once.end())
				{	once[mesh] = 1;

					to = path + name;
					if (CopyFile(from, to))
						++iVegetMesh;
					else
 						continue;
				}

				//  get mesh mtr
				//---------------------------------------------------------------------------------------
			#if 0  // NO, slow buggy, once for all
				try
				{
					String resGrp = "MeshV1";
					v1::MeshPtr v1Mesh;
					ResourceGroupManager::getSingleton().addResourceLocation(path, "FileSystem", resGrp);
					v1Mesh = v1::MeshManager::getSingleton().load( mesh, resGrp,
						v1::HardwareBuffer::HBU_STATIC, v1::HardwareBuffer::HBU_STATIC );
					
					int si = v1Mesh->getNumSubMeshes();
					for (int i=0; i < si; ++i)
					{
						auto sm = v1Mesh->getSubMesh(i);
						String mtr = sm->getMaterialName();
						gui->Exp(CGui::TXT, mesh +" - "+ mtr);
					}
				}	
				catch (Exception ex)
				{
					String s = "Error: loading mesh: " + mesh + " \nfrom: " + path + "\n failed ! \n" + ex.what() + "\n";
					gui->Exp(CGui::WARN, s);
				}
			#endif

				//  write  ------
				if (l==0)
					veg << "\n// trees  yawFrom, yawTo,  scaleFrom, scaleTo,  highDensity,  distance1, distance2," <<
						"  meshName colormap densitymap  gridspacing collmesh\n";

				//trees 0, 360, 0.1, 0.12, 2, 60, 3000, fir05_30.mesh aspen-test.dds aspen_grass_density2.png 
				veg << "trees 0, 360,  ";
				veg << vg.minScale << ", " << vg.maxScale << ",  ";
				
				veg << "1.0,  ";
				// veg << vg.dens * sc->densTrees * cfg->treesMul << ",  ";  // no, in dens png

				// veg << ", 60, 1000, ";  // vis dist
				veg << pveg->visDist * 0.5f << ", " << pveg->farDist << ",  ";  // par .. todo
				// veg << vg.name << " none " << mapName;
				veg << name << " none " << mapName;
				
				// veg << " -2" << "\n";  // grid?
				veg << "\n";

			} // exists
		} // layers
		
		veg.close();

		gui->Exp(CGui::NOTE, "Veget meshes: " + toStr(iVegetMesh));
		delete[] rdGr;
		delete[] rdTr;

		gui->Exp(CGui::INFO, "Veget Time: " + fToStr(ti.getMilliseconds()/1000.f,1,3) + " s\n");
	} // hasVeget
}
