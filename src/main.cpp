#include <iostream>
#include <Render/Init.h> // TODO: internal
#include <Render/GLContext.h>
#include <Render/RenderWindow.h>
#include <Logger/Log.h>

#include <BSPMap.h>

#include <fstream>
#include <iostream>

using namespace LambdaCore;

int main(int argc, char **argv)
{
    Commons::Render::Init init; // TODO: internal
    
    Commons::Render::WindowParams params;
    params.width = 1024;
    params.height = 768;
    params.title = "LambdaCore";
    
    Commons::Render::RenderWindow window(params);
    Commons::Render::GLContext context;

    // TODO: self stream object, exceptions or checks
    std::ifstream strm_map("f:/Games/Half-Life/valve/maps/gasworks.bsp", std::ifstream::binary);
    BSPMap map(strm_map);
    
    double oldTime = window.getCurTime();
    while (window.tick())
    {
        double curTime = window.getCurTime();
        float delta = static_cast<float>(curTime - oldTime);
        oldTime = curTime;
        
        //controller.update(delta);
        //render.render(camera, rootNode, window);
        window.swapBuffers();
    }

	return 0;
}


//#include <fstream>

//int main(int argc, char **argv)
//{
//	using namespace Commons;
//    using namespace OpenCarma;
//    using namespace OpenCarma::BRender;
//
//    try
//    {
//		LOG_INFO("Initializing Carma");
//
//		// TODO: parse arguments properly
//		std::string carmaPath;
//		if (argc > 1)
//		{
//			carmaPath = argv[1];
//		}
//		
//        Init init; // TODO: rm it!
//
//		Commons::Render::WindowParams params;
//		params.width = 1024;
//		params.height = 768;
//		params.title = "Carma";
//
//		Commons::Render::RenderWindow window(params);
//        Commons::Render::GLRender render;
//		OpenCarma::Render::Render carmaRender; // TODO: single
//
//        // TODO Load mdl
//        std::vector<ModelPtr> models;
//        std::ifstream strm_model(carmaPath + "/DATA/MODELS/EAGBLAK.DAT", std::ifstream::binary);
//        ModelSerializer::DeserializeModels(strm_model, models);
//
//        std::vector<PixmapPtr> palettes;
//        std::ifstream strm_palette(carmaPath + "/DATA/REG/PALETTES/DRRENDER.PAL", std::ifstream::binary);
//        TextureSerializer::DeserializePixelmap(strm_palette, palettes);
//        assert(!palettes.empty());
//    
//        std::vector<PixmapPtr> pixelmaps;
//        std::ifstream strm_pixmaps(carmaPath + "/DATA/PIXELMAP/EAGREDL8.PIX", std::ifstream::binary);
//        TextureSerializer::DeserializePixelmap(strm_pixmaps, pixelmaps);
//
//        std::vector<MaterialPtr> materials;
//        std::ifstream strm_material(carmaPath + "/DATA/MATERIAL/AGENTO.MAT", std::ifstream::binary);
//        MaterialSerializer::DeserializeMaterial(strm_material, materials);
//        
//        std::vector<ActorPtr> actors;
//        std::ifstream strm_act(carmaPath + "/DATA/ACTORS/BUSTER.ACT", std::ifstream::binary);
//        ActorSerializer::DeserializeActor(strm_act, actors);
//
//        //ScenePtr scene(std::make_shared<Scene>());
//		Commons::Render::RenderNodePtr rootNode = std::make_shared<Commons::Render::RenderNode>("root");
//		OpenCarma::Render::StaticModelPtr mdl(std::make_shared<OpenCarma::Render::StaticModel>(models[0], carmaRender));
//
//        rootNode->attachChild(std::dynamic_pointer_cast<Commons::Render::RenderNode>(mdl));
//		mdl->setTranslation(glm::translate(0.F, 0.F, 0.F));
//
//		//Commons::Render::RenderNodePtr axis = std::make_shared<AxisDrawable>();
//		//mdl->setVisible(false);
//		//rootNode->attachChild(axis);
//
//
//		Commons::Render::CameraPtr camera = std::make_shared<Commons::Render::Camera>();
//		// TODO: constructor?
//		camera->setPerspective(55.F, 4.F / 3.F, 1000.F, 0.01F);
//		FreeFlyCameraController controller(camera);
//        //render.getTextureManager().load(pixelmaps, palettes[0]);
//			
//		double oldTime = window.getCurTime();
//		bool isClosed = false;
//		while (window.tick())
//		{
//			double curTime = window.getCurTime();
//			float delta = static_cast<float>(curTime - oldTime);
//			oldTime = curTime;
//
//			controller.update(delta);
//			render.render(camera, rootNode, window);
//			window.swapBuffers();
//		}
//        
//        std::cout << "Carma finished" << std::endl;
//        return 0;
//    }
//   /* catch (const Exception& e)
//    {
//        std::cout << "Exception occurred: " << e.what() << std::endl;
//        return -1;
//    }*/
//    catch (const std::runtime_error& e)
//    {
//        std::cout << "Exception occurred: " << e.what() << std::endl;
//        return -1;
//    }
//}