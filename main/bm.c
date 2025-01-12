/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Bitmap and palette loading functions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pstypes.h"
#include "inferno.h"
#include "gr.h"
#include "bm.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "iff.h"
#include "cfile.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "makesig.h"
#include "interp.h"
#include "console.h"
#include "rle.h"

ubyte Sounds[MAX_SOUNDS];
ubyte AltSounds[MAX_SOUNDS];

//for each model, a model number for dying & dead variants, or -1 if none
int Dying_modelnums[MAX_POLYGON_MODELS];
int Dead_modelnums[MAX_POLYGON_MODELS];

//the polygon model number to use for the marker
int	Marker_model_num = -1;

//right now there's only one player ship, but we can have another by
//adding an array and setting the pointer to the active ship.
player_ship only_player_ship,*Player_ship=&only_player_ship;

//----------------- Miscellaneous bitmap pointers ---------------
int             Num_cockpits = 0;
bitmap_index    cockpit_bitmap[N_COCKPIT_BITMAPS];

//---------------- Variables for wall textures ------------------
int             Num_tmaps;
tmap_info       TmapInfo[MAX_TEXTURES];

//---------------- Variables for object textures ----------------

int             First_multi_bitmap_num=-1;

int             N_ObjBitmaps;
bitmap_index    ObjBitmaps[MAX_OBJ_BITMAPS];
ushort          ObjBitmapPtrs[MAX_OBJ_BITMAPS];     // These point back into ObjBitmaps, since some are used twice.

/*
 * reads n tmap_info structs from a CFILE
 */
int tmap_info_read_n(tmap_info *ti, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		ti[i].flags = cfile_read_byte(fp);
		ti[i].pad[0] = cfile_read_byte(fp);
		ti[i].pad[1] = cfile_read_byte(fp);
		ti[i].pad[2] = cfile_read_byte(fp);
		ti[i].lighting = cfile_read_fix(fp);
		ti[i].damage = cfile_read_fix(fp);
		ti[i].eclip_num = cfile_read_short(fp);
		ti[i].destroyed = cfile_read_short(fp);
		ti[i].slide_u = cfile_read_short(fp);
		ti[i].slide_v = cfile_read_short(fp);
	}
	return i;
}

void gamedata_close()
{
	free_polygon_models();
	bm_free_extra_objbitmaps();
	free_endlevel_data();
	rle_cache_close();
	piggy_close();
}

//-----------------------------------------------------------------
// Initializes game properties data (including texture caching system) and sound data.
int gamedata_init()
{
	init_polygon_models();
	init_endlevel();

		if (!properties_init())				// This calls properties_read_cmp
				Error("Cannot open ham file\n");

	piggy_read_sounds();

	return 0;
}

void bm_read_all(CFILE * fp)
{
	int i,t;

	NumTextures = cfile_read_int(fp);
	bitmap_index_read_n(Textures, NumTextures, fp );
	tmap_info_read_n(TmapInfo, NumTextures, fp);

	t = cfile_read_int(fp);
	cfread( Sounds, sizeof(ubyte), t, fp );
	cfread( AltSounds, sizeof(ubyte), t, fp );

	Num_vclips = cfile_read_int(fp);
	vclip_read_n(Vclip, Num_vclips, fp);

	Num_effects = cfile_read_int(fp);
	eclip_read_n(Effects, Num_effects, fp);

	Num_wall_anims = cfile_read_int(fp);
	wclip_read_n(WallAnims, Num_wall_anims, fp);

	N_robot_types = cfile_read_int(fp);
	robot_info_read_n(Robot_info, N_robot_types, fp);

	N_robot_joints = cfile_read_int(fp);
	jointpos_read_n(Robot_joints, N_robot_joints, fp);

	N_weapon_types = cfile_read_int(fp);
	weapon_info_read_n(Weapon_info, N_weapon_types, fp, Piggy_hamfile_version);

	N_powerup_types = cfile_read_int(fp);
	powerup_type_info_read_n(Powerup_info, N_powerup_types, fp);

	N_polygon_models = cfile_read_int(fp);
	polymodel_read_n(Polygon_models, N_polygon_models, fp);

	for (i=0; i<N_polygon_models; i++ )
		polygon_model_data_read(&Polygon_models[i], fp);

	for (i = 0; i < N_polygon_models; i++)
		Dying_modelnums[i] = cfile_read_int(fp);
	for (i = 0; i < N_polygon_models; i++)
		Dead_modelnums[i] = cfile_read_int(fp);

	t = cfile_read_int(fp);
	bitmap_index_read_n(Gauges, t, fp);
	bitmap_index_read_n(Gauges_hires, t, fp);

	N_ObjBitmaps = cfile_read_int(fp);
	bitmap_index_read_n(ObjBitmaps, N_ObjBitmaps, fp);
	for (i = 0; i < N_ObjBitmaps; i++)
		ObjBitmapPtrs[i] = cfile_read_short(fp);

	player_ship_read(&only_player_ship, fp);

	Num_cockpits = cfile_read_int(fp);
	bitmap_index_read_n(cockpit_bitmap, Num_cockpits, fp);

//@@	cfread( &Num_total_object_types, sizeof(int), 1, fp );
//@@	cfread( ObjType, sizeof(byte), Num_total_object_types, fp );
//@@	cfread( ObjId, sizeof(byte), Num_total_object_types, fp );
//@@	cfread( ObjStrength, sizeof(fix), Num_total_object_types, fp );

	First_multi_bitmap_num = cfile_read_int(fp);

	Num_reactors = cfile_read_int(fp);
	reactor_read_n(Reactors, Num_reactors, fp);

	Marker_model_num = cfile_read_int(fp);

	//@@cfread( &N_controlcen_guns, sizeof(int), 1, fp );
	//@@cfread( controlcen_gun_points, sizeof(vms_vector), N_controlcen_guns, fp );
	//@@cfread( controlcen_gun_dirs, sizeof(vms_vector), N_controlcen_guns, fp );

	if (Piggy_hamfile_version < 3) {
		exit_modelnum = cfile_read_int(fp);
		destroyed_exit_modelnum = cfile_read_int(fp);
	}
	else
		exit_modelnum = destroyed_exit_modelnum = N_polygon_models;
}

//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
#define N_D2_ROBOT_TYPES		66
#define N_D2_ROBOT_JOINTS		1145
#define N_D2_POLYGON_MODELS     166
#define N_D2_OBJBITMAPS			422
#define N_D2_OBJBITMAPPTRS		502
#define N_D2_WEAPON_TYPES		62

extern int Num_bitmap_files;
int extra_bitmap_num = 0;

void bm_free_extra_objbitmaps()
{
	int i;

	if (!extra_bitmap_num)
		extra_bitmap_num = Num_bitmap_files;

	for (i = Num_bitmap_files; i < extra_bitmap_num; i++)
	{
		N_ObjBitmaps--;
		d_free(GameBitmaps[i].bm_data);
	}
	extra_bitmap_num = Num_bitmap_files;
}

void bm_free_extra_models()
{
	while (N_polygon_models > N_D2_POLYGON_MODELS)
		free_model(&Polygon_models[--N_polygon_models]);
	while (N_polygon_models > exit_modelnum)
		free_model(&Polygon_models[--N_polygon_models]);
}

//type==1 means 1.1, type==2 means 1.2 (with weapons)
void bm_read_extra_robots(const char *fname,int type)
{
	CFILE *fp;
	int t,i;
	int version;

	fp = cfopen(fname,"rb");
	if (!fp) {
		printf("bm_read_extra_robots: could not open %s\n", fname);
		return;
	}

	if (type == 2) {
		int sig;

		sig = cfile_read_int(fp);
		if (sig != MAKE_SIG('X','H','A','M'))
			return;
		version = cfile_read_int(fp);
	}
	else
		version = 0;
	(void)version; // NOTE: we do not need it, but keep it for possible further use

	bm_free_extra_models();
	bm_free_extra_objbitmaps();

	//read extra weapons

	t = cfile_read_int(fp);
	N_weapon_types = N_D2_WEAPON_TYPES+t;
	if (N_weapon_types >= MAX_WEAPON_TYPES)
		Error("Too many weapons (%d) in <%s>.  Max is %d.",t,fname,MAX_WEAPON_TYPES-N_D2_WEAPON_TYPES);
	weapon_info_read_n(&Weapon_info[N_D2_WEAPON_TYPES], t, fp, 3);

	//now read robot info

	t = cfile_read_int(fp);
	N_robot_types = N_D2_ROBOT_TYPES+t;
	if (N_robot_types >= MAX_ROBOT_TYPES)
		Error("Too many robots (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_TYPES-N_D2_ROBOT_TYPES);
	robot_info_read_n(&Robot_info[N_D2_ROBOT_TYPES], t, fp);

	t = cfile_read_int(fp);
	N_robot_joints = N_D2_ROBOT_JOINTS+t;
	if (N_robot_joints >= MAX_ROBOT_JOINTS)
		Error("Too many robot joints (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_JOINTS-N_D2_ROBOT_JOINTS);
	jointpos_read_n(&Robot_joints[N_D2_ROBOT_JOINTS], t, fp);

	t = cfile_read_int(fp);
	N_polygon_models = N_D2_POLYGON_MODELS+t;
	if (N_polygon_models >= MAX_POLYGON_MODELS)
		Error("Too many polygon models (%d) in <%s>.  Max is %d.",t,fname,MAX_POLYGON_MODELS-N_D2_POLYGON_MODELS);
	polymodel_read_n(&Polygon_models[N_D2_POLYGON_MODELS], t, fp);

	for (i=N_D2_POLYGON_MODELS; i<N_polygon_models; i++ )
		polygon_model_data_read(&Polygon_models[i], fp);

	for (i = N_D2_POLYGON_MODELS; i < N_polygon_models; i++)
		Dying_modelnums[i] = cfile_read_int(fp);
	for (i = N_D2_POLYGON_MODELS; i < N_polygon_models; i++)
		Dead_modelnums[i] = cfile_read_int(fp);

	t = cfile_read_int(fp);
	if (N_D2_OBJBITMAPS+t >= MAX_OBJ_BITMAPS)
		Error("Too many object bitmaps (%d) in <%s>.  Max is %d.",t,fname,MAX_OBJ_BITMAPS-N_D2_OBJBITMAPS);
	bitmap_index_read_n(&ObjBitmaps[N_D2_OBJBITMAPS], t, fp);

	t = cfile_read_int(fp);
	if (N_D2_OBJBITMAPPTRS+t >= MAX_OBJ_BITMAPS)
		Error("Too many object bitmap pointers (%d) in <%s>.  Max is %d.",t,fname,MAX_OBJ_BITMAPS-N_D2_OBJBITMAPPTRS);
	for (i = N_D2_OBJBITMAPPTRS; i < (N_D2_OBJBITMAPPTRS + t); i++)
		ObjBitmapPtrs[i] = cfile_read_short(fp);

	cfclose(fp);
}

int Robot_replacements_loaded = 0;

void load_robot_replacements(char *level_name)
{
	CFILE *fp;
	int t,i,j;
	char ifile_name[FILENAME_LEN];

	change_filename_extension(ifile_name, level_name, ".HXM" );

	fp = cfopen(ifile_name,"rb");

	if (!fp)		//no robot replacement file
		return;

	t = cfile_read_int(fp);			//read id "HXM!"
	if (t!= 0x21584d48)
		Error("ID of HXM! file incorrect");

	t = cfile_read_int(fp);			//read version
	if (t<1)
		Error("HXM! version too old (%d)",t);

	t = cfile_read_int(fp);			//read number of robots
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read robot number
		if (i<0 || i>=N_robot_types)
			Error("Robots number (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_robot_types-1);
		robot_info_read_n(&Robot_info[i], 1, fp);
	}

	t = cfile_read_int(fp);			//read number of joints
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read joint number
		if (i<0 || i>=N_robot_joints)
			Error("Robots joint (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_robot_joints-1);
		jointpos_read_n(&Robot_joints[i], 1, fp);
	}

	t = cfile_read_int(fp);			//read number of polygon models
	for (j=0;j<t;j++)
	{
		i = cfile_read_int(fp);		//read model number
		if (i<0 || i>=N_polygon_models)
			Error("Polygon model (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_polygon_models-1);

		free_model(&Polygon_models[i]);
		polymodel_read(&Polygon_models[i], fp);
		polygon_model_data_read(&Polygon_models[i], fp);

		Dying_modelnums[i] = cfile_read_int(fp);
		Dead_modelnums[i] = cfile_read_int(fp);
	}

	t = cfile_read_int(fp);			//read number of objbitmaps
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read objbitmap number
		if (i<0 || i>=MAX_OBJ_BITMAPS)
			Error("Object bitmap number (%d) out of range in (%s).  Range = [0..%d].",i,level_name,MAX_OBJ_BITMAPS-1);
		bitmap_index_read(&ObjBitmaps[i], fp);
	}

	t = cfile_read_int(fp);			//read number of objbitmapptrs
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read objbitmapptr number
		if (i<0 || i>=MAX_OBJ_BITMAPS)
			Error("Object bitmap pointer (%d) out of range in (%s).  Range = [0..%d].",i,level_name,MAX_OBJ_BITMAPS-1);
		ObjBitmapPtrs[i] = cfile_read_short(fp);
	}

	cfclose(fp);
	Robot_replacements_loaded = 1;
}


/*
 * Routines for loading exit models
 *
 * Used by d1 levels (including some add-ons), and by d2 shareware.
 * Could potentially be used by d2 add-on levels, but only if they
 * don't use "extra" robots...
 */

// formerly exitmodel_bm_load_sub
bitmap_index read_extra_bitmap_iff( char * filename )
{
	bitmap_index bitmap_num;
	grs_bitmap * new = &GameBitmaps[extra_bitmap_num];
	ubyte newpal[256*3];
	int iff_error;		//reference parm to avoid warning message

	bitmap_num.index = 0;

	//MALLOC( new, grs_bitmap, 1 );
	iff_error = iff_read_bitmap(filename,new,newpal);
	new->bm_handle=0;
	if (iff_error != IFF_NO_ERROR)		{
		con_printf(CON_DEBUG, "Error loading exit model bitmap <%s> - IFF error: %s\n", filename, iff_errormsg(iff_error));
		return bitmap_num;
	}

	if ( iff_has_transparency )
		gr_remap_bitmap_good( new, newpal, iff_transparent_color, 254 );
	else
		gr_remap_bitmap_good( new, newpal, -1, 254 );

	new->avg_color = 0;	//compute_average_pixel(new);

	bitmap_num.index = extra_bitmap_num;

	GameBitmaps[extra_bitmap_num++] = *new;

	//d_free( new );
	return bitmap_num;
}

// formerly load_exit_model_bitmap
grs_bitmap *bm_load_extra_objbitmap(char *name)
{
	Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);

	{
		ObjBitmaps[N_ObjBitmaps] = read_extra_bitmap_iff(name);

		if (ObjBitmaps[N_ObjBitmaps].index == 0)
		{
			char *name2 = d_strdup(name);
			*strrchr(name2, '.') = '\0';
			ObjBitmaps[N_ObjBitmaps] = read_extra_bitmap_d1_pig(name2);
			d_free(name2);
		}
		if (ObjBitmaps[N_ObjBitmaps].index == 0)
			return NULL;

		if (GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_w!=64 || GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_h!=64)
			Error("Bitmap <%s> is not 64x64",name);
		ObjBitmapPtrs[N_ObjBitmaps] = N_ObjBitmaps;
		N_ObjBitmaps++;
		Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);
		return &GameBitmaps[ObjBitmaps[N_ObjBitmaps-1].index];
	}
}

void ogl_cache_polymodel_textures(int model_num);

int load_exit_models()
{
	CFILE *exit_hamfile;
	int start_num;

	bm_free_extra_models();
	bm_free_extra_objbitmaps();

	start_num = N_ObjBitmaps;
	if (!bm_load_extra_objbitmap("steel1.bbm") ||
		!bm_load_extra_objbitmap("rbot061.bbm") ||
		!bm_load_extra_objbitmap("rbot062.bbm") ||
		!bm_load_extra_objbitmap("steel1.bbm") ||
		!bm_load_extra_objbitmap("rbot061.bbm") ||
		!bm_load_extra_objbitmap("rbot063.bbm"))
	{
		con_printf(CON_NORMAL, "Can't load exit models!\n");
		return 0;
	}

	exit_hamfile = PHYSFSX_openReadBuffered("exit.ham");

	if (exit_hamfile) {
		exit_modelnum = N_polygon_models++;
		destroyed_exit_modelnum = N_polygon_models++;
		polymodel_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polymodel_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
		Polygon_models[exit_modelnum].first_texture = start_num;
		Polygon_models[destroyed_exit_modelnum].first_texture = start_num+3;

		polygon_model_data_read(&Polygon_models[exit_modelnum], exit_hamfile);

		polygon_model_data_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);

		cfclose(exit_hamfile);

	} else if (cfexist("exit01.pof") && cfexist("exit01d.pof")) {

		exit_modelnum = load_polygon_model("exit01.pof", 3, start_num, NULL);
		destroyed_exit_modelnum = load_polygon_model("exit01d.pof", 3, start_num + 3, NULL);

		ogl_cache_polymodel_textures(exit_modelnum);
		ogl_cache_polymodel_textures(destroyed_exit_modelnum);
	}
	else if (cfexist(D1_PIGFILE))
	{
		int offset, offset2;
		int hamsize;

		exit_hamfile = cfopen(D1_PIGFILE, "rb");
		hamsize = cfilelength(exit_hamfile);
		switch (hamsize) { //total hack for loading models
		case D1_PIGSIZE:
			offset = 91848;     /* and 92582  */
			offset2 = 383390;   /* and 394022 */
			break;
		default:
		case D1_SHARE_BIG_PIGSIZE:
		case D1_SHARE_10_PIGSIZE:
		case D1_SHARE_PIGSIZE:
		case D1_10_BIG_PIGSIZE:
		case D1_10_PIGSIZE:
			Int3();             /* exit models should be in .pofs */
		case D1_OEM_PIGSIZE:
		case D1_MAC_PIGSIZE:
		case D1_MAC_SHARE_PIGSIZE:
			con_printf(CON_NORMAL, "Can't load exit models!\n");
			return 0;
		}
		cfseek(exit_hamfile, offset, SEEK_SET);
		exit_modelnum = N_polygon_models++;
		destroyed_exit_modelnum = N_polygon_models++;
		polymodel_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polymodel_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);
		Polygon_models[exit_modelnum].first_texture = start_num;
		Polygon_models[destroyed_exit_modelnum].first_texture = start_num+3;

		cfseek(exit_hamfile, offset2, SEEK_SET);
		polygon_model_data_read(&Polygon_models[exit_modelnum], exit_hamfile);
		polygon_model_data_read(&Polygon_models[destroyed_exit_modelnum], exit_hamfile);

		cfclose(exit_hamfile);
	} else {
		con_printf(CON_NORMAL, "Can't load exit models!\n");
		return 0;
	}

	return 1;
}

void compute_average_rgb(grs_bitmap *bm, fix *rgb)
{
	ubyte *buf;
	int i, x, y, color, count;
	fix t_rgb[3] = { 0, 0, 0 };

	rgb[0] = rgb[1] = rgb[2] = 0;

	if (!bm->bm_data)
		return;

	MALLOC(buf, ubyte, bm->bm_w*bm->bm_h);
	memset(buf,0,bm->bm_w*bm->bm_h);

	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = buf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
	}
	else
	{
		memcpy(buf, bm->bm_data, sizeof(unsigned char)*(bm->bm_w*bm->bm_h));
	}

	i = 0;
	for (x = 0; x < bm->bm_h; x++)
	{
		for (y = 0; y < bm->bm_w; y++)
		{
			color = buf[i++];
			t_rgb[0] = gr_palette[color*3];
			t_rgb[1] = gr_palette[color*3+1];
			t_rgb[2] = gr_palette[color*3+2];
			if (!(color == TRANSPARENCY_COLOR || (t_rgb[0] == t_rgb[1] && t_rgb[0] == t_rgb[2])))
			{
				rgb[0] += t_rgb[0];
				rgb[1] += t_rgb[1];
				rgb[2] += t_rgb[2];
				count++;
			}
		}
	}
	d_free(buf);
}
