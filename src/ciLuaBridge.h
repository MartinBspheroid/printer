




#include "cinder/app/App.h"
#include "cinder/Filesystem.h"
#include "cinder/Text.h"
#include "cinder/gl/Texture.h"


extern "C" {
# include "lua.h"
# include "lauxlib.h"
# include "lualib.h"
}
#include <LuaBridge.h>

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace luabridge;

class LuaError{
public:

	LuaError(){ mFont = Font("Arial", 16); }
	LuaError(string what){
		mFont = Font("Arial", 12);
		mErrorMsg = what;
		


	};
	void setError(string what){
		mErrorMsg = what;
		TextBox tbox = TextBox().alignment(TextBox::RIGHT).font(mFont);
		tbox.text(mErrorMsg);
		tbox.setColor(Color::white());
		tbox.setBackgroundColor(Color::black());
		mTextTexture = gl::Texture2d::create(tbox.render());
	}
	void draw(){
		draw(vec2(0, 0));
	}
	void draw(vec2 offset){

		if (mErrorMsg.size() != 0)
		{
			gl::setMatricesWindow(getWindowSize());
			gl::enableAlphaBlending();

			if (mTextTexture){
				gl::translate(offset);
				gl::draw(mTextTexture);
			}
		}
	}
private:
	gl::TextureRef mTextTexture;
	Font			mFont;
	string			mErrorMsg;
};


class ciLuaBridge
{
public:
	ciLuaBridge(bool useTexture = true);
	~ciLuaBridge();
	LuaRef get(std::string name){
		return getGlobal(LuaState, name.c_str());
	}
	void test(){

		LuaRef s = getGlobal(LuaState, "testString");
		LuaRef n = getGlobal(LuaState, "number");
		std::string luaString = s.cast<std::string>();
		int answer = n.cast<int>();
		console() << luaString << std::endl;
		console() << "And here's our number:" << answer << std::endl;
	}

	Namespace bind(){
		return luabridge::getGlobalNamespace(LuaState);
	}
	void load(const fs::path &path)
	{
		mSrciptPath = path;
		int result = luaL_dofile(LuaState, path.string().c_str());
		reportErrors(LuaState, result);
		lua_pcall(LuaState, 0, 0, 0);

	}
	float getFloat(const string& name){
		if (getGlobal(LuaState, name.c_str()).isNumber())
			return luabridge::getGlobal(LuaState, name.c_str()).cast<float>();
		else
			return 0;
	}

	void reportErrors(lua_State *L, int status){
		if (status != 0){

			if (!useTextureError)
			{
				ci::app::console() << "[ERR] " << lua_tostring(L, -1);
			}
			else
			{
				LE.setError(lua_tostring(L, -1));
			}
			lua_pop(L, 1);
		}
		else
		{
			LE.setError("");
		}
	}
	void reload(){
		load(mSrciptPath);
	}


	void drawErrors(){ LE.draw(); }
	void drawErrors(vec2 offset) { LE.draw(offset); }

private:

	bool useTextureError;
	lua_State *LuaState;
	fs::path mSrciptPath;
	string errors;
	LuaError LE;
};

ciLuaBridge::ciLuaBridge(bool useTexture)
{

	LuaState = luaL_newstate();
	luaL_openlibs(LuaState);
	(useTexture) ? useTextureError = true : useTextureError = false;

}

ciLuaBridge::~ciLuaBridge()
{
	lua_close(LuaState);
}