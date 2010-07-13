/*
 *
 * Routines for displaying HUD messages...
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "hudmsg.h"
#include "pstypes.h"
#include "u_mem.h"
#include "strutil.h"
#include "console.h"
#include "inferno.h"
#include "game.h"
#include "screens.h"
#include "gauges.h"
#include "physics.h"
#include "error.h"
#include "menu.h"           // For the font.
#include "collide.h"
#include "newdemo.h"
#include "player.h"
#include "gamefont.h"
#include "wall.h"
#include "screens.h"
#include "text.h"
#include "laser.h"
#include "args.h"
#include "playsave.h"

typedef struct hudmsg
{
	fix time;
	char message[HUD_MESSAGE_LENGTH+1];
} hudmsg;

hudmsg HUD_messages[HUD_MAX_NUM_STOR];


static int HUD_nmessages = 0;
int HUD_toolong = 0;
static int HUD_color = -1;

void HUD_clear_messages()
{
	HUD_nmessages = 0;
	HUD_toolong = 0;
	memset(&HUD_messages, 0, sizeof(struct hudmsg)*HUD_MAX_NUM_STOR);
	HUD_color = -1;
}


// ----------------------------------------------------------------------------
//	Writes a message on the HUD and checks its timer.
void HUD_render_message_frame()
{
	int i,j,y;

	HUD_toolong = 0;

	if (( HUD_nmessages < 0 ) || (HUD_nmessages > HUD_MAX_NUM_STOR))
		Int3(); // Get Rob!

	if (HUD_nmessages < 1 )
		return;

	for (i = 0; i < HUD_nmessages; i++)
	{
		HUD_messages[i].time -= FrameTime;
		// message expired - remove
		if (HUD_messages[i].time <= 0)
		{
			for (j = i; j < HUD_nmessages; j++)
			{
				if (j+1 < HUD_nmessages)
					memcpy(&HUD_messages[j], &HUD_messages[j+1], sizeof(struct hudmsg));
				else
					memset(&HUD_messages[j], 0, sizeof(struct hudmsg));
			}
			HUD_nmessages--;
		}
	}

	// display last $HUD_MAX_NUM_DISP messages on the list
	if (HUD_nmessages > 0 )
	{
		int startmsg = ((HUD_nmessages-HUD_MAX_NUM_DISP<0)?0:HUD_nmessages-HUD_MAX_NUM_DISP);
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);

		gr_set_curfont( GAME_FONT );
		y = FSPACY(1);

		if (Guided_missile[Player_num] && Guided_missile[Player_num]->type==OBJ_WEAPON && Guided_missile[Player_num]->id==GUIDEDMISS_ID &&
		Guided_missile[Player_num]->signature==Guided_missile_sig[Player_num] && PlayerCfg.GuidedInBigWindow)
			y+=LINE_SPACING;

		for (i=startmsg; i<HUD_nmessages; i++ )	{
			gr_set_fontcolor( HUD_color, -1);

			if (i == startmsg && strlen(HUD_messages[i].message) > 38)
				HUD_toolong = 1;
			gr_printf(0x8000,y, &HUD_messages[i].message[0] );
			y += LINE_SPACING;
		}
	}

	gr_set_curfont( GAME_FONT );
}

// Call to flash a message on the HUD.  Returns true if message drawn.
// (message might not be drawn if previous message was same)
int HUD_init_message_va(char * format, va_list args)
{
	int i, j;
	char message[HUD_MESSAGE_LENGTH+1] = "";

	vsnprintf(message, sizeof(char)*HUD_MESSAGE_LENGTH, format, args);

	// already in list - do not add again
	for (i = 0; i < HUD_nmessages; i++)
	{
		if (!strnicmp(message, HUD_messages[i].message, sizeof(char)*HUD_MESSAGE_LENGTH))
		{
			HUD_messages[i].time = F1_0*2; // keep redundant message in list
			if (i >= HUD_nmessages-HUD_MAX_NUM_DISP) // if redundant message on display, update them all
				for (i = HUD_nmessages-HUD_MAX_NUM_DISP, j = 1; i < HUD_nmessages; i++, j++)
					HUD_messages[i].time = F1_0*(j*2);
			return 0;
		}
	}

	// HACK!!!
	// clean message if necessary.
	// using placeholders may mess up message string and crash game(s).
	// block them also to prevent attacks from other clients.
	for (i = 0; i <= strlen(message); i++)
		if (message[i] == '%')
			message [i] = ' ';

	if (HUD_nmessages >= HUD_MAX_NUM_STOR)
	{
		HUD_nmessages = HUD_MAX_NUM_STOR; // unnecessary but just in case it might be bigger... which is impossible
		for (i = 0; i < HUD_nmessages-1; i++) 
		{
			memcpy(&HUD_messages[i], &HUD_messages[i+1], sizeof(struct hudmsg));
		}
	}
	else
	{
		HUD_nmessages++;
	}

	snprintf(HUD_messages[HUD_nmessages-1].message, sizeof(char)*HUD_MESSAGE_LENGTH, message);
	if (HUD_nmessages-HUD_MAX_NUM_DISP < 0)
		HUD_messages[HUD_nmessages-1].time = F1_0*3; // one message - display 3 secs
	else
		for (i = HUD_nmessages-HUD_MAX_NUM_DISP, j = 1; i < HUD_nmessages; i++, j++) // multiple messages - display 2 seconds each
			HUD_messages[i].time = F1_0*(j*2);
	

	if (HUD_color == -1)
		HUD_color = BM_XRGB(0,28,0);
	con_printf(CON_HUD, "%s\n", message);

	if (Newdemo_state == ND_STATE_RECORDING )
		newdemo_record_hud_message( message );

	return 1;
}


int HUD_init_message(int class_flag, char * format, ... )
{
	int ret;
	va_list args;

	if (GameArg.SysNoRedundancy && class_flag & HM_REDUNDANT)
		return 0;

	if (GameArg.MplTxtMessages && Game_mode & GM_MULTI && class_flag & HM_REDUNDANT)
		return 0;

	va_start(args, format);
	ret = HUD_init_message_va(format, args);
	va_end(args);

	return ret;
}


void player_dead_message(void)
{
	if (Player_exploded) {
		if ( Players[Player_num].lives < 2 )    {
			int x, y, w, h, aw;
			gr_set_curfont( HUGE_FONT );
			gr_get_string_size( TXT_GAME_OVER, &w, &h, &aw );
			w += 20;
			h += 8;
			x = (grd_curcanv->cv_bitmap.bm_w - w ) / 2;
			y = (grd_curcanv->cv_bitmap.bm_h - h ) / 2;
		
			Gr_scanline_darkening_level = 2*7;
			gr_setcolor( BM_XRGB(0,0,0) );
			gr_rect( x, y, x+w, y+h );
			Gr_scanline_darkening_level = GR_FADE_LEVELS;
		
			gr_printf(0x8000, (GHEIGHT - h)/2 + h/8, TXT_GAME_OVER );
		}
	
		gr_set_curfont( GAME_FONT );
		if (HUD_color == -1)
			HUD_color = BM_XRGB(0,28,0);
		gr_set_fontcolor( HUD_color, -1);
		gr_printf(0x8000, GHEIGHT-LINE_SPACING, TXT_PRESS_ANY_KEY);
	}
}
