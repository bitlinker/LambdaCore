#include <iostream>
#include <Render/Init.h> // TODO: internal
#include <Render/GLContext.h>
#include <Render/RenderWindow.h>
#include <Render/SharedTextureMgr.h>
#include <Logger/Log.h>

#include <WAD.h>
#include <BSPRender.h>
#include <MipTexLoader.h>
#include <Model.h>
#include <FreeFlyCameraController.h>

#include <Streams/FileStream.h>

#include <Render/BufferObject.h>
#include <Render/VertexArrayObject.h>

using namespace LambdaCore;

class BSPMapRender : public Commons::Render::RenderNode
{
public:
    BSPMapRender(BSPMap &map)
    {
        //Commons::Render::BufferObject buf;
    }
protected:
    virtual void doRender(const glm::mat4& matrix)  // TODO: pass context here?
    {
        // TODO: render here
    };
};

int main(int argc, char **argv)
{
    Commons::Render::Init init; // TODO: internal
    
    Commons::Render::WindowParams params;
    params.width = 1600;
    params.height = 900;
    params.isFullscreen = false;
    params.title = "LambdaCore";

    //Commons::Render::WindowParams params;
    //params.width = 1920;
    //params.height = 1080;
    //params.fsaaSamples = 8;
    //params.isFullscreen = true;
    //params.title = "LambdaCore";
    
    Commons::Render::RenderWindow window(params);
    Commons::Render::GLContext context;

    Commons::FileStreamPtr strm_mdl(new Commons::FileStream("f:/Games/Half-Life/valve/models/barney.mdl", Commons::FileStream::MODE_READ));
    ModelPtr model = std::make_shared<Model>(strm_mdl);

    // TODO: self stream object, exceptions or checks
    Commons::FileStreamPtr strm_map(new Commons::FileStream("f:/Games/Half-Life/valve/maps/crossfire.bsp", Commons::FileStream::MODE_READ));
    BSPMapPtr map = std::make_shared<BSPMap>(strm_map);

    Commons::Render::CameraPtr camera(new Commons::Render::Camera());
    camera->setPerspective(45.F, 16.F / 9.F, 10240.F, 16.0F); // TODO: configure to eliminate z-fighting

    //bool check = camera->getFrustum().isInFrustum(glm::vec3(0, 0, -50), 1);
    
    // TODO: in bsp renderer
    glm::mat4 STATIC_MAP_TRANSFORM = glm::rotate(90.F, -1.F, 0.F, 0.F);
    glm::vec4 spawnPos(-688, 512, -1645, 1.F);
    spawnPos = STATIC_MAP_TRANSFORM * spawnPos;

    camera->setTranslation(glm::vec3(spawnPos));
    FreeFlyCameraController camController(camera);

    Commons::FileStreamPtr strm_mainwad(new Commons::FileStream("f:/Games/Half-Life/valve/halflife.wad", Commons::FileStream::MODE_READ));
    WADPtr mainWad(new WAD(strm_mainwad));
    MipTexLoaderPtr wadTexLoader (new MipTexLoader(mainWad));
    Commons::Render::SharedTextureMgrPtr textureMgr(new Commons::Render::SharedTextureMgr());
    textureMgr->addLoader(wadTexLoader);

    //Commons::Render::SharedTexturePtr tex = textureMgr->getTexture("+0button1");

    //Commons::Render::TexturePtr tex(new Commons::Render::Texture());
    //texLoader.loadTexture(textureStream, tex);
    

    Commons::Render::RenderNodePtr rootNode(new Commons::Render::RenderNode("root"));

    // TODO: dbg
    ::glEnable(GL_BLEND);
    ::glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //::glEnable(GL_ALPHA_TEST);
    //::glAlphaFunc(GL_GREATER, 0.5F);

    ::glEnable(GL_DEPTH_TEST);
    ::glEnable(GL_CULL_FACE);
    ::glFrontFace(GL_CW); // TODO: invert faces then building model?

                          // TODO: remove then lightmap packaging will be done
    ::glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    BSPRender mapRender(map, textureMgr);
    //Commons::Render::RenderNodePtr mapNode(new BSPMapRender(map));
    //rootNode->attachChild(mapNode);
        
    double oldTime = window.getCurTime();
    while (window.tick())
    {
        double curTime = window.getCurTime();
        float delta = static_cast<float>(curTime - oldTime);
        oldTime = curTime;
        
        //tex->get()->bind(); // TODO: dbg
        context.render(camera, rootNode, window);
        camController.update(delta);
        mapRender.render(camera);

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