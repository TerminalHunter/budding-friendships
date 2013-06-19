#include "mlpbf/console.h"
#include "mlpbf/console/command.h"
#include "mlpbf/global.h"
#include "mlpbf/lua.h"
#include "mlpbf/resource.h"

#include <cstring>
#include <iostream>
#include <SFML/Graphics/Sprite.hpp>

namespace bf
{
namespace lua
{

/***************************************************************************/

struct Image
{
	res::TexturePtr texture;
	sf::Sprite sprite;
};

static const char * IMAGE_MT = "game.image";

static int image_load( lua_State * l )
{
	lua::Image * image = (lua::Image *) luaL_checkudata( l, 1, IMAGE_MT );
	
	image->texture = res::loadTexture( luaL_checkstring( l, 2 ) );
	image->sprite.setTexture( *image->texture );
	
	return 0;
}

static int image_move( lua_State * l )
{
	lua::Image * image = (lua::Image *) luaL_checkudata( l, 1, IMAGE_MT );
	float x = luaL_checknumber( l, 2 );
	float y = luaL_checknumber( l, 3 );
	
	image->sprite.move( x, y );
	
	return 0;
}

static int image_origin( lua_State * l )
{
	lua::Image * image = (lua::Image *) luaL_checkudata( l, 1, IMAGE_MT );
	
	if ( lua_gettop( l ) == 3 )
	{
		float x = luaL_checknumber( l, 2 );
		float y = luaL_checknumber( l, 3 );
		
		image->sprite.setOrigin( x, y );
	}
	
	const sf::Vector2f & origin = image->sprite.getOrigin();
	lua_pushnumber( l, origin.x );
	lua_pushnumber( l, origin.y );
	
	return 2;
}

static int image_position( lua_State * l )
{
	lua::Image * image = (lua::Image *) luaL_checkudata( l, 1, IMAGE_MT );
	
	if ( lua_gettop( l ) == 3 )
	{
		float x = luaL_checknumber( l, 2 );
		float y = luaL_checknumber( l, 3 );
		
		image->sprite.setPosition( x, y );
	}

	const sf::Vector2f & pos = image->sprite.getPosition();
	lua_pushnumber( l, pos.x );
	lua_pushnumber( l, pos.y );
	
	return 2;
}

static int image_rotate( lua_State * l )
{
	lua::Image * image = (lua::Image *) luaL_checkudata( l, 1, IMAGE_MT );
	
	if ( lua_gettop( l ) == 2 ) // rotate
		image->sprite.rotate( luaL_checknumber( l, 2 ) );
	
	lua_pushnumber( l, image->sprite.getRotation() );
	return 1;
}

// (1) image:scale()
// (2) image:scale( x, y )
// version (2) sets the scale of the image
// both versions return the scale x, y
static int image_scale( lua_State * l )
{
	lua::Image * image = (lua::Image *) luaL_checkudata( l, 1, IMAGE_MT );

	if ( lua_gettop( l ) == 3 ) // setScale
	{
		float x = luaL_checknumber( l, 2 );
		float y = luaL_checknumber( l, 3 );
		
		image->sprite.setScale( x, y );
		
		return 0;
	}

	const sf::Vector2f & scale = image->sprite.getScale();
	
	lua_pushnumber( l, scale.x );
	lua_pushnumber( l, scale.y );
	
	return 2;
}

static int image_size( lua_State * l )
{
	lua::Image * data = (lua::Image *) luaL_checkudata( l, 1, IMAGE_MT );
	sf::Vector2u size = data->texture->getSize();
	
	lua_pushinteger( l, size.x );
	lua_pushinteger( l, size.y );
	
	return 2;
}

static int image_free( lua_State * l )
{
	lua::Image * data = (lua::Image *) luaL_checkudata( l, 1, IMAGE_MT );
	data->~Image();
	std::clog << "image_free called" << std::endl;
	return 0;
}

static const struct luaL_Reg libimage_mt [] =
{
	{ "load", 	image_load },
	{ "move",		image_move },
	{ "origin",	image_origin },
	{ "position",	image_position },
	{ "rotate",	image_rotate },
	{ "scale", 	image_scale },
	{ "size", 	image_size },
	{ "__gc", 	image_free },
	{ NULL, 		NULL },
};

/***************************************************************************/

// game.newImage()
// creates a new image userdata
int game_newImage( lua_State * l )
{
	// create userdata and set metatable
	lua::Image * data = (lua::Image *) lua_newuserdata( l, sizeof( lua::Image ) );
	new (data) lua::Image();
	
	luaL_getmetatable( l, IMAGE_MT );
	lua_setmetatable( l, -2 );
	
	return 1;
}

// game.showText( text [, speaker ] )
// displays a dialogue box
int game_showText( lua_State * l )
{
	bf::showText( luaL_checkstring( l, 1 ), luaL_optstring( l, 2, "" ) );
	return 0;
}

static const struct luaL_Reg libgame[] = 
{
	{ "newImage", game_newImage },
	{ "showText", game_showText },
	{ NULL, NULL },
};

/***************************************************************************/

// console.write( str [, col ] )
// prints a line to the console
int console_write( lua_State * l )
{
	if ( lua_gettop( l ) >= 2 && lua_isnumber( l, 2 ) )
		Console::singleton().setBufferColor( lua_tonumber( l, 2 ) );
	else
		Console::singleton().setBufferColor( Console::INFO_COLOR );

	Console::singleton() << luaL_checkstring( l, 1 ) << con::endl;
	return 0;
}

// console.execute( str )
// executes a console commmand
// note: cannot execute lua console command
int console_execute( lua_State * l )
{
	Console::singleton().execute( luaL_checkstring( l, 1 ) );
	return 0;
}

int console_hook( lua_State * l )
{
	class LuaCommand : public con::Command
	{
		const std::string cmd;
		int ref;
		mutable lua_State * l;
		
		const std::string name() const 
		{ 
			return cmd; 
		}
		
		unsigned minArgs() const 
		{ 
			return 0;
		}
		
		void help( Console & ) const
		{
		}
		
		void execute( Console & c, const std::vector< std::string > & args ) const
		{
			lua_rawgeti( l, LUA_REGISTRYINDEX, ref );
			
			int numargs = 0;
			for ( const std::string & str : args )
			{
				lua_pushstring( l, str.c_str() );
				numargs++;
			}
			
			if ( lua_pcall( l, numargs, 0, 0 ) )
			{
				c << con::setcerr << lua_tostring( l, -1 ) << con::endl;
				lua_pop( l, 1 );
			}
		}
	
	public:
		LuaCommand( lua_State * L ) :
			cmd( lua_tostring( L, 1 ) ),
			ref( luaL_ref( L, LUA_REGISTRYINDEX ) ),
			l( L )
		{
		}
		
		~LuaCommand() { luaL_unref( l, LUA_REGISTRYINDEX, ref ); }		
	};
	
	luaL_checktype( l, 1, LUA_TSTRING );
	luaL_checktype( l, 2, LUA_TFUNCTION );
	
	con::Command * cmd = new LuaCommand( l );
	Console::singleton().addCommand( cmd );
	Console::singleton() << con::setcinfo << "Lua: hooked console function " << cmd->name() << con::endl;

	return 0;
}

static const struct luaL_Reg libconsole[] = 
{
	{ "write", console_write },
	{ "execute", console_execute },
	{ "hook", console_hook },
	{ NULL, NULL },
};

/***************************************************************************/

lua_State * newState()
{
	// create lua state
	lua_State * l = luaL_newstate();
	luaL_openlibs( l );
	
	// texture metatable
	luaL_newmetatable( l, IMAGE_MT );

	lua_pushvalue( l, -1 );
	lua_setfield( l, -2, "__index" );
	
	luaL_setfuncs( l, libimage_mt, 0 );
	lua_pop( l, 1 );
	
	// register custom libraries
	luaL_newlib( l, libgame );
	lua_setglobal( l, "game" );

	luaL_newlib( l, libconsole );
	lua_setglobal( l, "console" );
	
	return l;
}

/***************************************************************************/

} // namespace lua

} // namespace bf
