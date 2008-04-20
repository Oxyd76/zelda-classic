//--------------------------------------------------------
//  Zelda Classic
//  by Jeremy Craner, 1999-2000
//
//  guys.cc
//
//  "Guys" code (and other related stuff) for zelda.cc
//
//  Still has some hardcoded stuff that should be moved
//  out into defdata.cc for customizing the enemies.
//
//--------------------------------------------------------

#include <string.h>
#include <stdio.h>
#include "zc_alleg.h"
#include "guys.h"
#include "zelda.h"
#include "zsys.h"
#include "maps.h"
#include "link.h"
#include "subscr.h"
#include "ffscript.h"
#include "gamedata.h"

extern LinkClass   Link;
extern sprite_list  guys, items, Ewpns, Lwpns, Sitems, chainlinks, decorations;

bool repaircharge;
bool adjustmagic;
bool learnslash;
int itemindex;
int wallm_load_clk=0;
int sle_x,sle_y,sle_cnt,sle_clk;
int vhead=0;
int guyindex=0;

bool hasBoss();
void never_return(int index);
void playLevelMusic();

int random_layer_enemy()
{
  int cnt=count_layer_enemies();

  if(cnt==0)
  {
    return eNONE;
  }

  int ret=rand()%cnt;
  cnt=0;
  for (int i=0; i<6; ++i)
  {
    if (tmpscr->layermap[i]!=0)
    {
      mapscr *layerscreen=TheMaps+((tmpscr->layermap[i]-1)*MAPSCRS)+tmpscr->layerscreen[i];
      for (int j=0; j<10; ++j)
      {
        if (layerscreen->enemy[j]!=0)
        {
          if (cnt==ret)
          {
            return layerscreen->enemy[j];
          }
          ++cnt;
        }
      }
    }
  }
  return eNONE;
}

int count_layer_enemies()
{
  int cnt=0;
  for (int i=0; i<6; ++i)
  {
    if (tmpscr->layermap[i]!=0)
    {
      mapscr *layerscreen=TheMaps+((tmpscr->layermap[i]-1)*MAPSCRS)+tmpscr->layerscreen[i];
      for (int j=0; j<10; ++j)
      {
        if (layerscreen->enemy[j]!=0)
        {
          ++cnt;
        }
      }
    }
  }
  return cnt;
}

bool can_do_clock()
{
  if(watch || hasBoss() || (get_bit(quest_rules,qr_NOCLOCKS))) return false;
  if(items.idFirst(iClock)>=0) return false;
  return true;
}

bool m_walkflag(int x,int y,int special)
{
  int yg = (special==spw_floater)?8:0;
  int nb = get_bit(quest_rules, qr_NOBORDER) ? 16 : 0;

  if(x<16-nb || y<zc_max(16-yg-nb,0) || x>=240+nb || y>=160+nb)
    return true;

  if(isdungeon() || special==spw_wizzrobe)
  {
    if(y<32-yg || y>=144)
      return true;
    if(x<32 || x>=224)
      if(special!=spw_door)                                 // walk in door way
        return true;
  }

  switch(special)
  {
    case spw_clipbottomright: if(y>=128) return true;
    case spw_clipright: if(x>=208) return true; break;
    case spw_wizzrobe:
    case spw_floater: return false;
  }

  x&=(special==spw_halfstep)?(~7):(~15);
  y&=(special==spw_halfstep)?(~7):(~15);

  if(special==spw_water)
    //    return water_walkflag(x,y+8,1) && water_walkflag(x+8,y+8,1);
    return water_walkflag(x,y+8,1) || water_walkflag(x+8,y+8,1);

  //  return _walkflag(x,y+8,1) && _walkflag(x+8,y+8,1);
  return _walkflag(x,y+8,1) || _walkflag(x+8,y+8,1) ||
    COMBOTYPE(x,y+8)==cPIT || COMBOTYPE(x+8,y+8)==cPIT||
    COMBOTYPE(x,y+8)==cPITB || COMBOTYPE(x+8,y+8)==cPITB||
    COMBOTYPE(x,y+8)==cPITC || COMBOTYPE(x+8,y+8)==cPITC||
    COMBOTYPE(x,y+8)==cPITD || COMBOTYPE(x+8,y+8)==cPITD||
    COMBOTYPE(x,y+8)==cPITR || COMBOTYPE(x+8,y+8)==cPITR||
//	COMBOTYPE(x,y+8)==cOLD_NOENEMY || COMBOTYPE(x+8,y+8)==cOLD_NOENEMY||
	(combo_class_buf[COMBOTYPE(x,y+8)].block_enemies&1) || (combo_class_buf[COMBOTYPE(x+8,y+8)].block_enemies&1) ||
	MAPFLAG(x,y+8)==mfNOENEMY || MAPFLAG(x+8,y+8)==mfNOENEMY||
	MAPCOMBOFLAG(x,y+8)==mfNOENEMY || MAPCOMBOFLAG(x+8,y+8)==mfNOENEMY;

}

int link_on_wall()
{
  int lx = Link.getX();
  int ly = Link.getY();
  if(lx>=48 && lx<=192)
  {
    if(ly==32)  return up+1;
    if(ly==128) return down+1;
  }
  if(ly>=48 && ly<=112)
  {
    if(lx==32)  return left+1;
    if(lx==208) return right+1;
  }
  return 0;
}

bool tooclose(int x,int y,int d)
{
  return (abs(int(LinkX())-x)<d && abs(int(LinkY())-y)<d);
}

bool isflier(int id)
{
  switch (guysbuf[id&0xFFF].family)//id&0x0FFF)
  {
    //case ePEAHAT:
    //case eKEESE1:
    //case eKEESE2:
    //case eKEESE3:
    //case eKEESETRIB:
    //case eBAT:
    //case ePATRA1:
    //case ePATRA2:
    //case ePATRAL2:
    //case ePATRAL3:
    //case ePATRABS:
    //case eITEMFAIRY:
  case eePEAHAT:
  case eeKEESE:
  case eeKEESETRIB:
  case eePATRA:
  case eeFAIRY:
    return true;
    break;
  }
  return false;
}

bool isfloater(int id)
{
  switch (guysbuf[id].family)//id&0x0FFF)
  {
    //case eMANHAN:
    //case eMANHAN2:
  case eeMANHAN:
    return true;
    break;
  }
  return isflier(id);
}

/**********************************/
/*******  Enemy Base Class  *******/
/**********************************/

/* ROM data flags

  */

enemy::enemy(fix X,fix Y,int Id,int Clk) : sprite()
{
  x=X; y=Y; id=Id; clk=Clk;
  floor_y=y;
  fading = misc = clk2 = clk3 = stunclk = hclk = sclk = 0;
  grumble = movestatus = posframe = timer = ox = oy = 0;
  yofs = playing_field_offset - 2;

  d = guysbuf + (id & 255);
  hp = d->hp;
//  cs = d->cset;
//d variables

  flags=d->flags;
  flags2=d->flags2;
  s_tile=d->s_tile; //secondary (additional) tile(s)
  family=d->family;
  dcset=d->cset;
  cs=dcset;
  anim=get_bit(quest_rules,qr_NEWENEMYTILES)?d->e_anim:d->anim;
  dp=d->dp;
  wdp=d->wdp;
  wpn=d->weapon;

  rate=d->rate;
  hrate=d->hrate;
  dstep=d->step;
  homing=d->homing;
  dmisc1=d->misc1;
  dmisc2=d->misc2;
  dmisc3=d->misc3;
  dmisc4=d->misc4;
  dmisc5=d->misc5;
  dmisc6=d->misc6;
  dmisc7=d->misc7;
  dmisc8=d->misc8;
  dmisc9=d->misc9;
  dmisc10=d->misc10;
  bgsfx=d->bgsfx;
  bosspal=d->bosspal;

//  if (d->bosspal>-1)
  if (bosspal>-1)
  {
//    loadpalset(csBOSS,pSprite(d->bosspal));
    loadpalset(csBOSS,pSprite(bosspal));
  }
//  if (d->bgsfx>-1)
  if (bgsfx>-1)
  {
//    cont_sfx(d->bgsfx);
    cont_sfx(bgsfx);
  }
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    o_tile=d->e_tile;
    frate = d->e_frate*(get_bit(quest_rules,qr_SLOWENEMYANIM)?2:1);
  }
  else
  {
    o_tile=d->tile;
    frate = d->frate*(get_bit(quest_rules,qr_SLOWENEMYANIM)?2:1);
  }
  tile=0;

//  step = d->step/100.0;
  step = dstep/100.0;
  item_set = d->item_set;
  grumble = d->grumble;
  if(frate == 0)
    frate = 256;

  superman = !!(flags & guy_superman);
  if (superman)
    superman += !!(flags & guy_sbombonly);

  leader = itemguy = dying = scored = false;
  canfreeze = count_enemy = mainguy = true;
  dir = rand()&3;

  // For now, at least...
  if (wpn==ewBrang && family!=eeGORIYA)
    wpn=0;
}

enemy::~enemy() {}

// Supplemental animation code that all derived classes should call
// as a return value for animate().
// Handles the death animation and returns true when enemy is finished.
bool enemy::Dead(int index)
{
  if(dying)
  {
    --clk2;
    if(clk2==12 && hp>-1000)                                // not killed by ringleader
      death_sfx();
    if(clk2==0)
    {
//      if(index>-1 && (d->flags&guy_neverret))
      if(index>-1 && (flags&guy_neverret))
        never_return(index);
      if(leader)
        kill_em_all();
      leave_item();
      return true;
    }
    if (bgsfx > 0 && guys.idCount(id) < 2) // count self
      stop_sfx(bgsfx);
  }
  return false;
}

// Basic animation code that all derived classes should call.
// The one with an index is the one that is called by
// the guys sprite list; index is the enemy's index in the list.
bool enemy::animate(int )
{
  int nx = real_x(x);
  int ny = real_y(y);
  if(ox!=nx || oy!=ny)
  {
    posframe=(posframe+1)%(get_bit(quest_rules,qr_NEWENEMYTILES)?4:2);
  }
  ox = nx;
  oy = ny;

  //fall down
  if (canfall(id) && fading != fade_flicker && clk>0)
  {
    if (tmpscr->flags7&fSIDEVIEW)
     {
       if (!_walkflag(x,y+16,0))
       {
         y+=(int)fall/100;
         if (y>186)
	  kickbucket();
	 else if (fall <= TERMINALV)
	   fall += GRAVITY;
       }
       else
	 fall = 0;
     }
     else
     {
       if (fall!=0)
	 z-=(int)fall/100;

       if (z<0)
         z = fall = 0;
       else if (fall <= TERMINALV)
         fall += GRAVITY;
    }
  }

  // clk is incremented here
  if(++clk >= frate)
    clk=0;
  // hit and death handling
  if(hclk>0)
    --hclk;
  if(stunclk>0)
    --stunclk;

  if(!dying && hp<=0)
  {
    if (itemguy && hasitem==2)
    {
      items.spr(itemindex)->x = x;
      items.spr(itemindex)->y = y - 2;
    }
    dying=true;
    if(fading==fade_flash_die)
      clk2=19+18*4;
    else
    {
      clk2 = BSZ ? 15 : 19;
      if(fading!=fade_blue_poof)
        fading=0;
    }
    if(itemguy)
    {
      hasitem=0;
      item_set=0;
    }
    if(currscr<128 && count_enemy)
      game->guys[(currmap<<7)+currscr]-=1;
  }

  scored=false;

  ++c_clk;

  // returns true when enemy is defeated
  return Dead();
}

// to allow for different sfx on defeating enemy
void enemy::death_sfx()
{
  sfx(WAV_EDEAD,pan(int(x)));
}

void enemy::move(fix dx,fix dy)
{
  if(!watch)
  {
    x+=dx;
    y+=dy;
  }
}

void enemy::move(fix s)
{
  if(!watch)
    sprite::move(s);
}

item_drop_object item_drop_set[256]=
{
  { "None",            0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                                               { 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
  { "Default",         6, { iFairyMoving, i10Rupies, i5Rupies, i1ArrowAmmo, iHeart, iRupy, 0, 0, 0, 0 },                  { 60, 3, 3, 5, 7, 12, 20, 0, 0, 0, 0 } },
  { "Bombs",           8, { iFairyMoving, iClock, i10Rupies, iRupy, i5Rupies, i1ArrowAmmo, iHeart, iBombs, 0, 0 },        { 58, 2, 4, 4, 10, 4, 5, 10, 12, 0, 0 } },
  { "Money",           7, { iFairyMoving, i10Rupies, iClock, i1ArrowAmmo, i5Rupies, iHeart, iRupy, 0, 0, 0 },             { 45, 3, 3, 5, 7, 15, 10, 22, 0, 0, 0 } },
  { "Life",            3, { iFairyMoving, iRupy, iHeart, 0, 0, 0, 0, 0, 0, 0 },                                           { 52, 8, 8, 32, 0, 0, 0, 0, 0, 0, 0 } },
  { "Bomb 100%",       1, { iBombs, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                                          { 0, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
  { "Super Bomb 100%", 1, { iSBomb, 0, 0, 0, 0, 0, 0, 0, 0, 0 },                                                          { 0, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
  { "Magic",           2, { iLMagic, iSMagic, 0, 0, 0, 0, 0, 0, 0, 0 },                                                   { 76, 8, 16, 0, 0, 0, 0, 0, 0, 0, 0 } },
  { "Magic/Bombs",     9, { iFairyMoving, iLMagic, i10Rupies, iSMagic, iClock, iRupy, i5Rupies, iHeart, iBombs, 0 },      { 52, 2, 2, 2, 4, 4, 10, 4, 10, 12, 0 } },
  { "Magic/Money",     9, { iFairyMoving, iLMagic, i10Rupies, iSMagic, i1ArrowAmmo, iClock, i5Rupies, iHeart, iRupy, 0 }, { 36, 3, 3, 3, 6, 3, 5, 15, 10, 22, 0 } },
  { "Magic/Life",      7, { iFairyMoving, i10Rupies, iLMagic, i1ArrowAmmo, iSMagic, iRupy, iHeart, 0, 0, 0 },             { 20, 4, 2, 4, 4, 8, 16, 48, 0, 0, 0 } },
  { "Magic 100%",      2, { iLMagic, iSMagic, 0, 0, 0, 0, 0, 0, 0, 0 },                                                   { 0, 75, 25, 0, 0, 0, 0, 0, 0, 0, 0 } },
};


void enemy::leave_item()
{
  int total_chance=0;
  for (int k=0; k<item_drop_set[item_set].items+1; ++k)
  {
    int current_chance=item_drop_set[item_set].chance[k];

    if (k>0)
    {
      int current_item=item_drop_set[item_set].item[k-1];
      if ((!get_bit(quest_rules,qr_ENABLEMAGIC)||(game->get_maxmagic()<=0))&&(current_item==iSMagic||current_item==iLMagic))
      {
        current_chance=0;
      }
      if ((!get_bit(quest_rules,qr_ALLOW10RUPEEDROPS))&&(current_item==i10Rupies))
      {
        current_chance=0;
      }
      if ((!get_bit(quest_rules,qr_TRUEARROWS))&&(current_item==i1ArrowAmmo))
      {
        current_chance=0;
      }
	  if ((get_bit(quest_rules,qr_NOCLOCKS))&&(current_item==iClock))
	  {
		current_chance=0;
      }
    }
    total_chance+=current_chance;
  }
  int item_chance=(rand()%total_chance)+1;
  int drop_item=-1;
  for (int k=item_drop_set[item_set].items; k>0; --k)
  {
    int current_chance=item_drop_set[item_set].chance[k];
    int current_item=item_drop_set[item_set].item[k-1];
    if ((!get_bit(quest_rules,qr_ENABLEMAGIC)||(game->get_maxmagic()<=0))&&(current_item==iSMagic||current_item==iLMagic))
    {
      current_chance=0;
    }
    if ((!get_bit(quest_rules,qr_ALLOW10RUPEEDROPS))&&(current_item==i10Rupies))
    {
      current_chance=0;
    }
    if ((!get_bit(quest_rules,qr_TRUEARROWS))&&(current_item==i1ArrowAmmo))
    {
      current_chance=0;
    }
	if ((get_bit(quest_rules,qr_NOCLOCKS))&&(current_item==iClock))
    {
      current_chance=0;
    }
    if (current_chance>0&&item_chance<=current_chance)
    {
      drop_item=current_item;
      break;
    }
    else
    {
      item_chance-=current_chance;
    }
  }

  if (drop_item==iFairyMoving)
  {
    for (int j=0; j<items.Count(); ++j)
    {
      if ((items.spr(j)->id==iFairyMoving)&&((abs(items.spr(j)->x-x)<32)||(abs(items.spr(j)->y-y)<32)))
      {
        drop_item=-1;
        break;
      }
    }
  }

  if (drop_item!=-1&&(drop_item!=iFairyMoving||!m_walkflag(x,y,0)))
  {
    items.add(new item(x,y,(fix)0,drop_item,ipBIGRANGE+ipTIMER,0));
  }
}

// auomatically kill off enemy (for rooms with ringleaders)
void enemy::kickbucket()
{
  if(!superman)
    hp=-1000;                                               // don't call death_sfx()
}


// take damage or ignore it
int enemy::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0)
    return 0;
  if(superman && !(wpnId==wSBomb && superman==2)            // vulnerable to super bombs
                                                            // fire boomerang, for nailing untouchable enemies
     && !(wpnId==wBrang && current_item_power(itype_brang)>0))
    return 0;

  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    return 0;
    case wFSparkle:
    takehit(wSword, DAMAGE_MULTIPLIER>>1, wpnx, wpny, wpnDir);
    break;
    case wBrang:
    if(!(flags & guy_bhit))
    {
      stunclk=160;
      if (current_item_power(itype_brang))
      {
        takehit(wSword,current_item_power(itype_brang)*DAMAGE_MULTIPLIER, wpnx, wpny, wpnDir);
      }
      break;
    }
    if (!power)
      hp-=current_item(itype_brang)*DAMAGE_MULTIPLIER;
    else
      hp-=power;
    goto hitclock;
    case wHookshot:
    if(!(flags & guy_bhit))
    {
      stunclk=160;
      if (current_item_power(itype_hookshot))
      {
        takehit(wSword,current_item_power(itype_hookshot)*DAMAGE_MULTIPLIER, wpnx, wpny, wpnDir);
      }
      break;
    }
    if (!power) hp-=current_item(itype_hookshot)*DAMAGE_MULTIPLIER;
    else
      hp-=power;
    goto hitclock;
    case wHSHandle:
    if (itemsbuf[current_item_id(itype_hookshot)].flags & ITEM_MISC1_PER)
      return 0;
    if(!(flags & guy_bhit))
    {
      stunclk=160;
      if (current_item_power(itype_hookshot))
      {
        takehit(wSword,current_item_power(itype_hookshot)*DAMAGE_MULTIPLIER, wpnx, wpny, wpnDir);
      }
      break;
    }
    if (!power) hp-=current_item(itype_hookshot)*DAMAGE_MULTIPLIER;
    else
      hp-=power;
    goto hitclock;
    default:
    if (!power) {
      if(!(flags & guy_bhit))
	hp-=1;
      else {
        stunclk=160;
	break;
      }
    }
    else hp-=power;
hitclock:
    hclk=33;
    if((dir&2)==(wpnDir&2))
      sclk=(wpnDir<<8)+16;
  }

  if(((wpnId==wBrang) || (get_bit(quest_rules,qr_NOFLASHDEATH))) && hp<=0)
  {
    fading=fade_blue_poof;
  }

  sfx(WAV_EHIT, pan(int(x)));
  if (guysbuf[id&0xFF].family==eeGUY) // NES consistency
    sfx(WAV_EDEAD, pan(int(x)));
  // arrow keeps going
  return ((wpnId==wArrow)&&(current_item_power(itype_arrow)>4)) ? 0 : 1;
  //    return 1;
}

bool enemy::dont_draw()
{
  if(fading==fade_invisible || (fading==fade_flicker && (clk&1)))
    return true;
  if(flags&guy_invisible)
    return true;
  if(flags&lens_only && !lensclk)
    return true;
  return false;
}

// base drawing function to be used by all derived classes instead of
// sprite::draw()
void enemy::draw(BITMAP *dest)
{
  if(dont_draw())
    return;

  int cshold=cs;
  if(dying)
  {
    if(clk2>=19)
    {
      if(!(clk2&2))
        sprite::draw(dest);
      return;
    }
    flip = 0;
    tile = wpnsbuf[iwDeath].tile;

    if(BSZ)
      tile += zc_min((15-clk2)/3,4);
    else if(clk2>6 && clk2<=12)
        ++tile;

      /* trying to get more death frames here
        if(wpnsbuf[wid].frames)
        {
        if(++clk2 >= wpnsbuf[wid].speed)
        {
        clk2 = 0;
        if(++aframe >= wpnsbuf[wid].frames)
        aframe = 0;
        }
        tile = wpnsbuf[wid].tile + aframe;
        }
        */

      if(BSZ || fading==fade_blue_poof)
      cs = wpnsbuf[iwDeath].csets&15;
    else
      cs = (((clk2+5)>>1)&3)+6;
  }
  else if(hclk>0 && (hclk<33 || family==eeGANON))
      cs=(((hclk-1)>>1)&3)+6;
  if ((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet)) && !(get_bit(quest_rules,qr_LENSSEESENEMIES) && lensclk) && family!=eeGANON)
  {
    sprite::drawcloaked(dest);
  }
  else
  {
    sprite::draw(dest);
  }
  cs=cshold;
}

// similar to the overblock function--can do up to a 32x32 sprite
void enemy::drawblock(BITMAP *dest,int mask)
{
  int thold=tile;
  int t1=tile;
  int t2=tile+20;
  int t3=tile+1;
  int t4=tile+21;
  switch(mask)
  {
    case 1:
    enemy::draw(dest);
    break;
    case 3:
    if(flip&2)
      zc_swap(t1,t2);
    tile=t1; enemy::draw(dest);
    tile=t2; yofs+=16; enemy::draw(dest); yofs-=16;
    break;
    case 5:
    t2=tile+1;
    if(flip&1)
      zc_swap(t1,t2);
    tile=t1; enemy::draw(dest);
    tile=t2; xofs+=16; enemy::draw(dest); xofs-=16;
    break;
    case 15:
    if(flip&1)
    {
      zc_swap(t1,t3);
      zc_swap(t2,t4);
    }
    if(flip&2)
    {
      zc_swap(t1,t2);
      zc_swap(t3,t4);
    }
    tile=t1; enemy::draw(dest);
    tile=t2; yofs+=16; enemy::draw(dest); yofs-=16;
    tile=t3; xofs+=16; enemy::draw(dest);
    tile=t4; yofs+=16; enemy::draw(dest); xofs-=16; yofs-=16;
    break;
  }
  tile=thold;
}

void enemy::drawshadow(BITMAP *dest, bool translucent)
{
  if(dont_draw() || tmpscr->flags7&fSIDEVIEW)
  {
    return;
  }

  if(dying)
  {
    return;
  }
  if (((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet)))||
      (darkroom))
  {
    return;
  }
  else
  {
 /*   if (canfall(id) && z>0)
      shadowtile = wpnsbuf[iwShadow].tile;
    sprite::drawshadow(dest,translucent);
    if (z==0)
      shadowtile = 0;*/
	// a bad idea, as enemies do their own setting of the shadow tile (since some use the
	// 2x2 tiles, shadows animate, etc.) -DD

	  //this hack is in place as not all enemies that should use the z axis while in the air
	  //(ie rocks, boulders) actually do. To be removed when the enemy revamp is complete -DD
    if(canfall(id) && shadowtile == 0)
		shadowtile = wpnsbuf[iwShadow].tile;
    if(z>0 || !canfall(id))
		sprite::drawshadow(dest,translucent);
  }
}

void enemy::masked_draw(BITMAP *dest,int mx,int my,int mw,int mh)
{
  BITMAP *sub=create_sub_bitmap(dest,mx,my,mw,mh);
  if(sub!=NULL)
  {
    xofs-=mx;
    yofs-=my;
    enemy::draw(sub);
    xofs+=mx;
    yofs+=my;
    destroy_bitmap(sub);
  }
  else
    enemy::draw(dest);
}

// override hit detection to check for invicibility, stunned, etc
bool enemy::hit(sprite *s)
{
  return (dying || hclk>0) ? false : sprite::hit(s);
}

bool enemy::hit(int tx,int ty,int tz,int txsz,int tysz,int tzsz)
{
  return (dying || hclk>0) ? false : sprite::hit(tx,ty,tz,txsz,tysz,tzsz);
}

bool enemy::hit(weapon *w)
{
  return (dying || hclk>0) ? false : sprite::hit(w);
}

//                         --==**==--

//   Movement routines that can be used by derived classes as needed

//                         --==**==--

void enemy::fix_coords()
{
  x=(fix)((int(x)&0xF0)+((int(x)&8)?16:0));
  y=(fix)((int(y)&0xF0)+((int(y)&8)?16:0));
}

// returns true if next step is ok, false if there is something there
bool enemy::canmove(int ndir,fix s,int special,int dx1,int dy1,int dx2,int dy2)
{
  bool ok;
  switch(ndir)
  {
    case 8:
    case up:
     if (canfall(id) && tmpscr->flags7&fSIDEVIEW)
       return false;
     ok = !m_walkflag(x,y+dy1-s,(special==spw_clipbottomright)?spw_none:special) && !((special==spw_floater)&&((COMBOTYPE(x,y+dy1-s)==cNOFLYZONE)||(combo_class_buf[COMBOTYPE(x,y+dy1-s)].block_enemies&1)||(MAPFLAG(x,y+dy1-s)==mfNOENEMY)||(MAPCOMBOFLAG(x,y+dy1-s)==mfNOENEMY)));
     break;
    case 12:
    case down:
     if (canfall(id) && tmpscr->flags7&fSIDEVIEW)
       return false;
     ok = !m_walkflag(x,y+dy2+s,special) && !((special==spw_floater)&&((COMBOTYPE(x,y+dy2+s)==cNOFLYZONE)||(COMBOTYPE(x,y+dy2+s)==cNOENEMY)||(MAPFLAG(x,y+dy2+s)==mfNOENEMY)||(MAPCOMBOFLAG(x,y+dy2+s)==mfNOENEMY)));
     break;
    case 14:
    case left:
      ok = !m_walkflag(x+dx1-s,y+8,(special==spw_clipbottomright||special==spw_clipright)?spw_none:special) && !((special==spw_floater)&&((COMBOTYPE(x+dx1-s,y+8)==cNOFLYZONE)||(COMBOTYPE(x+dx1-s,y+8)==cNOENEMY)||(MAPFLAG(x+dx1-s,y+8)==mfNOENEMY)||(MAPCOMBOFLAG(x+dx1-s,y+8)==mfNOENEMY)));
      break;
    case 10:
    case right:
      ok = !m_walkflag(x+dx2+s,y+8,special) && !((special==spw_floater)&&((COMBOTYPE(x+dx2+s,y+8)==cNOFLYZONE)||(COMBOTYPE(x+dx2+s,y+8)==cNOENEMY)||(MAPFLAG(x+dx2+s,y+8)==mfNOENEMY)||(MAPCOMBOFLAG(x+dx2+s,y+8)==mfNOENEMY)));
      break;
    case 9:
    case r_up:
      ok = !m_walkflag(x,y+dy1-s,special) && !m_walkflag(x+dx2+s,y+8,special) && !((special==spw_floater)&&((COMBOTYPE(x,y+dy1-s)==cNOFLYZONE)||(COMBOTYPE(x,y+dy1-s)==cNOENEMY)||(MAPFLAG(x,y+dy1-s)==mfNOENEMY)||(MAPCOMBOFLAG(x,y+dy1-s)==mfNOENEMY))) && !((special==spw_floater)&&((COMBOTYPE(x+dx2+s,y+8)==cNOFLYZONE)||(COMBOTYPE(x+dx2+s,y+8)==cNOENEMY)||(MAPFLAG(x+dx2+s,y+8)==mfNOENEMY)||(MAPCOMBOFLAG(x+dx2+s,y+8)==mfNOENEMY)));
      break;
    case 11:
    case r_down:
      ok = !m_walkflag(x,y+dy2+s,special) && !m_walkflag(x+dx2+s,y+8,special) && !((special==spw_floater)&&((COMBOTYPE(x,y+dy2+s)==cNOFLYZONE)||(COMBOTYPE(x,y+dy2+s)==cNOENEMY)||(MAPFLAG(x,y+dy2+s)==mfNOENEMY)||(MAPCOMBOFLAG(x,y+dy2+s)==mfNOENEMY))) && !((special==spw_floater)&&((COMBOTYPE(x+dx2+s,y+8)==cNOFLYZONE)||(COMBOTYPE(x+dx2+s,y+8)==cNOENEMY)||(MAPFLAG(x+dx2+s,y+8)==mfNOENEMY)||(MAPCOMBOFLAG(x+dx2+s,y+8)==mfNOENEMY)));
      break;
    case 13:
    case l_down:
      ok = !m_walkflag(x,y+dy2+s,special) && !m_walkflag(x+dx1-s,y+8,special) && !((special==spw_floater)&&((COMBOTYPE(x,y+dy2+s)==cNOFLYZONE)||(COMBOTYPE(x,y+dy2+s)==cNOENEMY)||(MAPFLAG(x,y+dy2+s)==mfNOENEMY)||(MAPCOMBOFLAG(x,y+dy2+s)==mfNOENEMY))) && !((special==spw_floater)&&((COMBOTYPE(x+dx1-s,y+8)==cNOFLYZONE)||(COMBOTYPE(x+dx1-s,y+8)==cNOENEMY)||(MAPFLAG(x+dx1-s,y+8)==mfNOENEMY)||(MAPCOMBOFLAG(x+dx1-s,y+8)==mfNOENEMY)));
      break;
    case 15:
    case l_up:
      ok = !m_walkflag(x,y+dy1-s,special) && !m_walkflag(x+dx1-s,y+8,special) && !((special==spw_floater)&&((COMBOTYPE(x,y+dy1-s)==cNOFLYZONE)||(COMBOTYPE(x,y+dy1-s)==cNOENEMY)||(MAPFLAG(x,y+dy1-s)==mfNOENEMY)||(MAPCOMBOFLAG(x,y+dy1-s)==mfNOENEMY))) && !((special==spw_floater)&&((COMBOTYPE(x+dx1-s,y+8)==cNOFLYZONE)||(COMBOTYPE(x+dx1-s,y+8)==cNOENEMY)||(MAPFLAG(x+dx1-s,y+8)==mfNOENEMY)||(MAPCOMBOFLAG(x+dx1-s,y+8)==mfNOENEMY)));
      break;
    default:
      db=99;
      ok=true;
      break;
  }
  return ok;
}


bool enemy::canmove(int ndir,fix s,int special)
{
  return canmove(ndir,s,special,0,-8,15,15);
}

bool enemy::canmove(int ndir,int special)
{
  return canmove(ndir,(fix)1,special,0,-8,15,15);
}

bool enemy::canmove(int ndir)
{
  return canmove(ndir,(fix)1,spw_none,0,-8,15,15);
}

// 8-directional
void enemy::newdir_8(int newrate,int special,int dx1,int dy1,int dx2,int dy2)
{
  int ndir=0;
  // can move straight, check if it wants to turn
  if(canmove(dir,step,special,dx1,dy1,dx2,dy2))
  {
    int r=rand();
    if(newrate>0 && !(r%newrate))
    {
      ndir = ((dir+((r&64)?-1:1))&7)+8;
      if(canmove(ndir,step,special,dx1,dy1,dx2,dy2))
        dir=ndir;
      else
      {
        ndir = ((dir+((r&64)?1:-1))&7)+8;
        if(canmove(ndir,step,special,dx1,dy1,dx2,dy2))
          dir=ndir;
      }
      if(dir==ndir)
      {
        x.v&=0xFFFF0000;
        y.v&=0xFFFF0000;
      }
    }
    return;
  }
  // can't move straight, must turn
  int i=0;
  for( ; i<32; i++)
  {
    ndir=(rand()&7)+8;
    if(canmove(ndir,step,special,dx1,dy1,dx2,dy2))
      break;
  }
  if(i==32)
  {
    for(ndir=8; ndir<16; ndir++)
    {
      if(canmove(i,step,special,dx1,dy1,dx2,dy2))
        goto ok;
    }
    ndir = -1;
  }

ok:
  dir=ndir;
  x.v&=0xFFFF0000;
  y.v&=0xFFFF0000;
}

void enemy::newdir_8(int newrate,int special)
{
  newdir_8(newrate,special,0,-8,15,15);
}

// makes the enemy slide backwards when hit
// sclk: first byte is clk, second byte is dir
bool enemy::slide()
{
  if(sclk==0 || hp<=0)
    return false;
  if((sclk&255)==16 && !canmove(sclk>>8,(fix)12,0))
  {
    sclk=0;
    return false;
  }
  --sclk;
  switch(sclk>>8)
  {
    case up:    y-=4; break;
    case down:  y+=4; break;
    case left:  x-=4; break;
    case right: x+=4; break;
  }
  if(!canmove(sclk>>8,(fix)0,0))
  {
    switch(sclk>>8)
    {
      case up:
      case down:
      if( (int(y)&15) > 7 )
        y=(int(y)&0xF0)+16;
      else
        y=(int(y)&0xF0);
      break;
      case left:
      case right:
      if( (int(x)&15) > 7 )
        x=(int(x)&0xF0)+16;
      else
        x=(int(x)&0xF0);
      break;
    }
    sclk=0;
    clk3=0;
  }
  if((sclk&255)==0)
    sclk=0;
  return true;
}


bool enemy::fslide()
{
  if(sclk==0 || hp<=0)
    return false;
  if((sclk&255)==16 && !canmove(sclk>>8,(fix)12,spw_floater))
  {
    sclk=0;
    return false;
  }
  --sclk;
  switch(sclk>>8)
  {
    case up:    y-=4; break;
    case down:  y+=4; break;
    case left:  x-=4; break;
    case right: x+=4; break;
  }
  if(!canmove(sclk>>8,(fix)0,spw_floater))
  {
    switch(sclk>>8)
    {
      case up:
      case down:
      if( (int(y)&15) > 7 )
        y=(int(y)&0xF0)+16;
      else
        y=(int(y)&0xF0);
      break;
      case left:
      case right:
      if( (int(x)&15) > 7 )
        x=(int(x)&0xF0)+16;
      else
        x=(int(x)&0xF0);
      break;
    }
    sclk=0;
    clk3=0;
  }
  if((sclk&255)==0)
    sclk=0;
  return true;
}

// changes enemy's direction, checking restrictions
// rate:   0 = no random changes, 16 = always random change
// homing: 0 = none, 256 = always
// grumble 0 = none, 4 = strongest appetite
void enemy::newdir(int newrate,int newhoming,int special)
{
  int ndir=-1;
  if(grumble && (rand()&3)<grumble)
    //  for super bait
    //    if(1)
  {
    int w = Lwpns.idFirst(wBait);
    if(w>=0)
    {
      int bx = Lwpns.spr(w)->x;
      int by = Lwpns.spr(w)->y;
      if(abs(int(y)-by)>14)
      {
        ndir = (by<y) ? up : down;
        if(canmove(ndir,special))
        {
          dir=ndir;
          return;
        }
      }
      ndir = (bx<x) ? left : right;
      if(canmove(ndir,special))
      {
        dir=ndir;
        return;
      }
    }
  }
  if((rand()&255)<newhoming)
  {
    ndir = lined_up(8,false);
    if(ndir>=0 && canmove(ndir,special))
    {
      dir=ndir;
      return;
    }
  }

  int i=0;
  for( ; i<32; i++)
  {
    int r=rand();
    if((r&15)<newrate)
      ndir=(r>>4)&3;
    else
      ndir=dir;
    if(canmove(ndir,special))
      break;
  }
  if(i==32)
  {
    for(ndir=0; ndir<4; ndir++)
    {
      if(canmove(ndir,special))
        goto ok;
    }
    ndir = -1;
  }

ok:
  dir = ndir;
}

void enemy::newdir()
{
  newdir(4,0,spw_none);
}

fix enemy::distance_left()
{
  int a2=x.v>>16;
  int b2=y.v>>16;

  switch(dir)
  {
    case up:    return (fix)(b2&0xF);
    case down:  return (fix)(16-(b2&0xF));
    case left:  return (fix)(a2&0xF);
    case right: return (fix)(16-(a2&0xF));
  }
  return (fix)0;
}

// keeps walking around
void enemy::constant_walk(int newrate,int newhoming,int special)
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  if(clk3<=0)
  {
    fix_coords();
    newdir(newrate,newhoming,special);
	if(step==0)
		clk3=0;
	else
		clk3=int(16.0/step);
  }
  else if(scored)
  {
    dir^=1;
    clk3=int(16.0/step)-clk3;
  }
  --clk3;
  move(step);
}

void enemy::constant_walk()
{
  constant_walk(4,0,spw_none);
}

int enemy::pos(int newx,int newy)
{
  return (newy<<8)+newx;
}

// for variable step rates
void enemy::variable_walk(int newrate,int newhoming,int special)
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  if((int(x)&15)==0 && (int(y)&15)==0 && clk3!=pos(x,y))
  {
    fix_coords();
    newdir(newrate,newhoming,special);
    clk3=pos(x,y);
  }
  move(step);
}

// pauses for a while after it makes a complete move (to a new square)
void enemy::halting_walk(int newrate,int newhoming,int special,int newhrate, int haltcnt)
{
  if(sclk && clk2)
  {
    clk3=0;
  }
  if(slide() || clk<0 || dying || stunclk || watch)
  {
    return;
  }
  if(clk2>0)
  {
    --clk2;
    return;
  }
  if(clk3<=0)
  {
    fix_coords();
    newdir(newrate,newhoming,special);
    clk3=int(16.0/step);
    if(clk2<0)
    {
      clk2=0;
    }
    else if((rand()&15)<newhrate)
    {
      clk2=haltcnt;
      return;
    }
  }
  else if(scored)
  {
    dir^=1;

    clk3=int(16.0/step)-clk3;
  }
  --clk3;
  move(step);
}

// 8-directional movement, aligns to 8 pixels
void enemy::constant_walk_8(int newrate,int special)
{
  if(clk<0 || dying || stunclk || watch)
    return;
  if(clk3<=0)
  {
    newdir_8(newrate,special);
    clk3=int(8.0/step);
  }
  --clk3;
  move(step);
}

// 8-directional movement, no alignment
void enemy::variable_walk_8(int newrate,int newclk,int special)
{
  if(clk<0 || dying || stunclk || watch)
    return;
  if(!canmove(dir,step,special))
    clk3=0;
  if(clk3<=0)
  {
    newdir_8(newrate,special);
    clk3=newclk;
  }
  --clk3;
  move(step);
}

// same as above but with variable enemy size
void enemy::variable_walk_8(int newrate,int newclk,int special,int dx1,int dy1,int dx2,int dy2)
{
  if(clk<0 || dying || stunclk || watch)
    return;
  if(!canmove(dir,step,special,dx1,dy1,dx2,dy2))
    clk3=0;
  if(clk3<=0)
  {
    newdir_8(newrate,special,dx1,dy1,dx2,dy2);
    clk3=newclk;
  }
  --clk3;
  move(step);
}

// the variable speed floater movement
// ms is max speed
// ss is step speed
// s is step count
// p is pause count
// g is graduality :)

void enemy::floater_walk(int newrate,int newclk,fix ms,fix ss,int s,int p, int g)
{
  ++clk2;
  switch(movestatus)
  {
    case 0:                                                 // paused
    if(clk2>=p)
    {
      movestatus=1;
      clk2=0;
    }
    break;

    case 1:                                                 // speeding up
    if(clk2<g*s)
    {
      if(!((clk2-1)%g))
        step+=ss;
    }
    else
    {
      movestatus=2;
      clk2=0;
    }
    break;

    case 2:                                                 // normal
    step=ms;
    if(clk2>48 && !(rand()%768))
    {
      step=ss*s;
      movestatus=3;
      clk2=0;
    }
    break;

    case 3:                                                 // slowing down
    if(clk2<=g*s)
    {
      if(!(clk2%g))
        step-=ss;
    }
    else
    {
      movestatus=0;
      clk2=0;
    }
    break;
  }
  variable_walk_8(movestatus==2?newrate:0,newclk,spw_floater);
}

void enemy::floater_walk(int newrate,int newclk,fix s)
{
  floater_walk(newrate,newclk,s,(fix)0.125,3,80,32);
}

// Checks if enemy is lined up with Link. If so, returns direction Link is
// at as compared to enemy. Returns -1 if not lined up. Range is inclusive.
int enemy::lined_up(int range, bool dir8)
{
  int lx = Link.getX();
  int ly = Link.getY();
  if(abs(lx-int(x))<=range)
  {
    if(ly<y)
    {
      return up;
    }
    return down;
  }
  if(abs(ly-int(y))<=range)
  {
    if(lx<x)
    {
      return left;
    }
    return right;
  }

  if (dir8)
  {
    if (abs(lx-x)-abs(ly-y)<=range)
    {
      if (ly<y)
      {
        if (lx<x)
        {
          return l_up;
        }
        else
        {
          return r_up;
        }
      }
      else
      {
        if (lx<x)
        {
          return l_down;
        }
        else
        {
          return r_down;
        }
      }
    }
  }

  return -1;
}

// returns true if Link is within 'range' pixels of the enemy
bool enemy::LinkInRange(int range)
{
  int lx = Link.getX();
  int ly = Link.getY();
  return abs(lx-int(x))<=range && abs(ly-int(y))<=range;
}

// place the enemy in line with Link (red wizzrobes)
void enemy::place_on_axis(bool floater)
{
  int lx=zc_min(zc_max(int(Link.getX())&0xF0,32),208);
  int ly=zc_min(zc_max(int(Link.getY())&0xF0,32),128);
  int pos2=rand()%23;
  int tried=0;
  bool last_resort,placed=false;

  do
  {
    if(pos2<14)
    { x=(pos2<<4)+16; y=ly; }
    else
    { x=lx; y=((pos2-14)<<4)+16; }

    if(x<32 || y<32 || x>=224 || y>=144)
      last_resort=false;
    else
      last_resort=true;

    if(abs(lx-int(x))>16 || abs(ly-int(y))>16)
    {
      if(!m_walkflag(x,y,1))

        placed=true;
      else if(floater && last_resort && !iswater(MAPCOMBO(x,y)))
          placed=true;
    }

    if(!placed && tried>=22 && last_resort)
      placed=true;
    ++tried;
    pos2=(pos2+3)%23;
  } while(!placed);

  if(y==ly)
    dir=(x<lx)?right:left;
  else
    dir=(y<ly)?down:up;
  clk2=tried;
}


void enemy::tiledir_small(int ndir)
{
  flip=0;
  switch (ndir)
  {
    case 8:
    case up:
    break;
    case 12:
    case down:
    tile+=2;
    break;
    case 14:
    case left:
    tile+=4;
    break;
    case 10:
    case right:
    tile+=6;
    break;
    case 9:
    case r_up:
    tile+=10;
    break;
    case 11:
    case r_down:
    tile+=14;
    break;
    case 13:
    case l_down:
    tile+=12;
    break;
    case 15:
    case l_up:
    tile+=8;
    break;
    default:
    //dir=(rand()*100)%8;
    //tiledir_small(dir);
    //      flip=rand()&3;
    //      tile=(rand()*100000)%NEWMAXTILES;
    break;
  }
}

void enemy::tiledir(int ndir)
{
  flip=0;
  switch (ndir)
  {
    case 8:
    case up:
    break;
    case 12:
    case down:
    tile+=4;
    break;
    case 14:
    case left:
    tile+=8;
    break;
    case 10:
    case right:
    tile+=12;
    break;
    case 9:
    case r_up:
    tile+=24;
    break;
    case 11:
    case r_down:
    tile+=32;
    break;
    case 13:
    case l_down:
    tile+=28;
    break;
    case 15:
    case l_up:
    tile+=20;
    break;
    default:
    //dir=(rand()*100)%8;
    //tiledir(dir);
    //      flip=rand()&3;
    //      tile=(rand()*100000)%NEWMAXTILES;
    break;
  }
}

void enemy::tiledir_big(int ndir)
{
  flip=0;
  switch (ndir)
  {
    case 8:
    case up:
    flip=0;
    break;
    case 12:
    case down:
    tile+=8;
    break;
    case 14:
    case left:
    tile+=40;
    break;
    case 10:
    case right:
    tile+=48;
    break;
    case 9:
    case r_up:
    tile+=88;
    break;
    case 11:
    case r_down:
    tile+=128;
    break;
    case 13:
    case l_down:
    tile+=120;
    break;
    case 15:
    case l_up:
    tile+=80;
    break;
    default:
    //dir=(rand()*100)%8;
    //tiledir_big(dir);
    //      flip=rand()&3;
    //      tile=(rand()*100000)%NEWMAXTILES;
    break;
  }
}

void enemy::update_enemy_frame()
{
	int newfrate = zc_max(frate,4);
  int f4=clk/(newfrate/4);
  int f2=clk/(newfrate/(get_bit(quest_rules,qr_NEWENEMYTILES)?4:2));
  tile = o_tile;
  switch(anim)
  {
    case aDONGO:
    {
      int fr = stunclk>0 ? 16 : 8;
      if(!dying && clk2>0 && clk2<=64)
      {
        // bloated
        switch(dir)
        {
          case up:
          tile+=9;
          flip=0;
          xofs=0;
          dummy_int[1]=0; //no additional tiles
          break;
          case down:
          tile+=7;
          flip=0;
          xofs=0;
          dummy_int[1]=0; //no additional tiles
          break;
          case left:
          flip=1;
          tile+=4;
          xofs=16;
          dummy_int[1]=1; //second tile is next tile
          break;
          case right:
          flip=0;
          tile+=5;
          xofs=16;
          dummy_int[1]=-1; //second tile is previous tile
          break;
        }
      }
      else if(!dying || clk2>19)
      {
        // normal
        switch(dir)
        {
          case up:
          tile+=8;
          flip=(clk&fr)?1:0;
          xofs=0;
          dummy_int[1]=0; //no additional tiles
          break;
          case down:
          tile+=6;
          flip=(clk&fr)?1:0;
          xofs=0;
          dummy_int[1]=0; //no additional tiles
          break;
          case left:
          flip=1;
          tile+=(clk&fr)?2:0;
          xofs=16;
          dummy_int[1]=1; //second tile is next tile
          break;
          case right:
          flip=0;
          tile+=(clk&fr)?3:1;
          xofs=16;
          dummy_int[1]=-1; //second tile is next tile
          break;
        }
      }
    }
    break;
    case aNEWDONGO:
    {
      int fr4=0;
      if(!dying && clk2>0 && clk2<=64)
      {
        // bloated
        if (clk2>=0)
        {
          fr4=3;
        }
        if (clk2>=16)
        {
          fr4=2;
        }
        if (clk2>=32)
        {
          fr4=1;
        }
        if (clk2>=48)
        {
          fr4=0;
        }
        switch(dir)
        {
          case up:
            xofs=0;
            tile+=8+fr4;
            dummy_int[1]=0; //no additional tiles
            break;
          case down:
            xofs=0;
            tile+=12+fr4;
            dummy_int[1]=0; //no additional tiles
            break;
          case left:
            tile+=29+(2*fr4);
            xofs=16;
            dummy_int[1]=-1; //second tile is previous tile
            break;
          case right:
            tile+=49+(2*fr4);
            xofs=16;
            dummy_int[1]=-1; //second tile is previous tile
            break;
        }
      }
      else if(!dying || clk2>19)
      {
        // normal
        switch(dir)
        {
          case up:
            xofs=0;
            tile+=((clk&12)>>2);
            dummy_int[1]=0; //no additional tiles
            break;
          case down:
            xofs=0;
            tile+=4+((clk&12)>>2);
            dummy_int[1]=0; //no additional tiles
            break;
          case left:
            tile+=21+((clk&12)>>1);
            xofs=16;
            dummy_int[1]=-1; //second tile is previous tile
            break;
          case right:
            flip=0;
            tile+=41+((clk&12)>>1);
            xofs=16;
            dummy_int[1]=-1; //second tile is previous tile
            break;
        }
      }
    }
    break;
    case aDONGOBS:
    {
      int fr4=0;
      if(!dying && clk2>0 && clk2<=64)
      {
        // bloated
        if (clk2>=0)
        {
          fr4=3;
        }
        if (clk2>=16)
        {
          fr4=2;
        }
        if (clk2>=32)
        {
          fr4=1;
        }
        if (clk2>=48)
        {
          fr4=0;
        }
        switch(dir)
        {
          case up:
          tile+=28+fr4;
          yofs+=8;
          dummy_int[1]=-20; //second tile change
          dummy_int[2]=0;   //new xofs change
          dummy_int[3]=-16; //new xofs change
          break;
          case down:
          tile+=12+fr4;
          yofs-=8;
          dummy_int[1]=20; //second tile change
          dummy_int[2]=0;  //new xofs change
          dummy_int[3]=16; //new xofs change
          break;
          case left:
          tile+=49+(2*fr4);
          xofs+=8;
          dummy_int[1]=-1; //second tile change
          dummy_int[2]=-16; //new xofs change
          dummy_int[3]=0;  //new xofs change
          break;
          case right:
          tile+=69+(2*fr4);
          xofs+=8;
          dummy_int[1]=-1; //second tile change
          dummy_int[2]=-16; //new xofs change
          dummy_int[3]=0;  //new xofs change
          break;
        }
      }
      else if(!dying || clk2>19)
      {
        // normal
        switch(dir)
        {
          case up:
          tile+=20+((clk&24)>>3);
          yofs+=8;
          dummy_int[1]=-20; //second tile change
          dummy_int[2]=0;   //new xofs change
          dummy_int[3]=-16; //new xofs change
          break;
          case down:
          tile+=4+((clk&24)>>3);
          yofs-=8;
          dummy_int[1]=20; //second tile change
          dummy_int[2]=0;  //new xofs change
          dummy_int[3]=16; //new xofs change
          break;
          case left:
          xofs=-8;
          tile+=40+((clk&24)>>2);
          dummy_int[1]=1; //second tile change
          dummy_int[2]=16; //new xofs change
          dummy_int[3]=0; //new xofs change
          break;
          case right:
          tile+=60+((clk&24)>>2);
          xofs=-8;
          dummy_int[1]=1; //second tile change
          dummy_int[2]=16; //new xofs change
          dummy_int[3]=0; //new xofs change
          break;
        }
      }
    }
    break;
    case aWIZZ:
    {
//      if(d->misc1)
      if(dmisc1)
      {
        if (clk&8)
        {
          ++tile;
        }
      }
      else
      {
        if (frame&4)
        {
          ++tile;
        }
      }

      switch(dir)
      {
        case 9:
        case 15:
        case up:
        tile+=2;
        break;
        case down:
        break;
        case 13:
        case left:  flip=1; break;
        default:    flip=0; break;
      }
    }
    break;
    case aNEWWIZZ:
    {
      tiledir(dir);
//      if(d->misc1)                                            //walking wizzrobe
      if(dmisc1)                                            //walking wizzrobe
      {
        if (clk&8)
        {
          tile+=2;
        }
        if (clk&4)
        {
          tile+=1;
        }
        if (!(dummy_bool[1]||dummy_bool[2]))                              //should never be charging or firing for these wizzrobes
        {
          if (dummy_int[1]>0)
          {
            tile+=40;
          }
        }
      }
      else
      {
        if (dummy_bool[1]||dummy_bool[2])
        {
          tile+=20;
          if (dummy_bool[2])
          {
            tile+=20;
          }
        }
        tile+=((frame>>1)&3);
      }
    }
    break;
    case a3FRM:
    {
      if (f4==3)
      {
        f4=1;
      }
      tile+=f4;
    }
    break;
    case aNEWPOLV:
    {
      tiledir(dir);
      tile+=f2;
      if (clk2>=0)
      {
        tile+=20;
      }
    }
    break;
    case aVIRE:
    {
      if(dir==up)
      {
        tile+=2;
      }
      tile+=f2;
    }
    break;
    case aROPE:
    {
      tile+=(1-f2);
      flip = dir==left ? 1:0;
    }
    break;
    case aZORA:
    {
      int dl;
      if(clk<36)
      {
        dl=clk+5;
        goto waves2;
      }
      if(clk<36+66)
        tile=(dir==up)?113:112;
      else
      {
        dl=clk-36-66;
     waves2:
        tile=((dl/11)&1)+96;
      }
    }
    break;
    case aNEWZORA:
    {
      f2=(clk/16)%4;

      tiledir(dir);
      int dl;
      if ((clk>35)&&(clk<36+67))                              //surfaced
      {
        if ((clk>=(35+10))&&(clk<(38+56)))                    //mouth open
        {
          tile+=80;
        }                                                     //mouth closed
        else
        {
          tile+=40;
        }
        tile+=f2;
      }
      else
      {
        if (clk<36)
        {
          dl=clk+5;
        }
        else
        {
          dl=clk-36-66;
        }
        tile+=((dl/5)&3);
      }
    }
    break;
    case a4FRM8EYE:
    {
      double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
      int lookat=rand()&15;
      if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
      {
        lookat=l_down;
      }
      else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
      {
        lookat=down;
      }
      else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
      {
        lookat=r_down;
      }
      else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
      {
        lookat=right;
      }
      else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
      {
        lookat=r_up;
      }
      else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
      {
        lookat=up;
      }
      else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
      {
        lookat=l_up;
      }
      else
      {
        lookat=left;
      }
      tiledir(lookat);
      tile+=f2;
    }
    break;
    case aFLIP:
    {
      flip = f2&1;
    }
    break;
    case aFLIPSLOW:
    {
      flip = (f2>>1)&1;
    }
    break;
    case a2FRM:
    {
      tile += (1-f2);
    }
    break;
	case a2FRMB:
	{
		tile+= 2*(1-f2);
	}
	break;
    case a2FRMSLOW:
    {
      tile += (1-(f2>>1));
    }
    break;
    case a2FRM4DIR:
    {
      tiledir(dir);
      if (clk2>0)                                             //stopped to fire
      {
        tile+=20;
        if (clk2<17)                                          //firing
        {
          tile+=20;
        }
      }
      tile+=f2;
    }
    break;
    case a4FRM4DIRF:
    {
      tiledir(dir);
      if (clk2>0)                                             //stopped to fire
      {
        tile+=20;
        if (clk2<17)                                          //firing
        {
          tile+=20;
        }
      }
      tile+=f2;
    }
    break;
    case a4FRM4DIR:
    {
      tiledir(dir);
      tile+=f2;
    }
    break;
    case a4FRM8DIRF:
    {
      tiledir(dir);
      if (clk2>0)                                             //stopped to fire
      {
        tile+=40;
        if (clk2<17)                                          //firing
        {
          tile+=40;
        }
      }
      tile+=f2;
    }
    break;
    case a4FRM8DIRB:
    {
      tiledir_big(dir);

      tile+=2*f2;
    }
    break;
    case aOCTO:
    {
      switch(dir)
      {
        case up:    flip = 2; break;
        case down:  flip = 0; break;
        case left:  flip = 0; tile += 2; break;
        case right: flip = 1; tile += 2; break;
      }
      tile+=f2;
    }
    break;
    case aWALK:
    {
      switch(dir)
      {
        case up:    tile+=3; flip = f2; break;
        case down:  tile+=2; flip = f2; break;
        case left:  flip=1; tile += f2; break;
        case right: flip=0; tile += f2; break;
      }
    }
    break;
    case aDWALK:
    {
      if ((get_bit(quest_rules,qr_BRKNSHLDTILES)) && (dummy_bool[1]==true))
      {
        tile=s_tile;
      }
      switch(dir)
      {
        case up:    tile+=2; flip=f2; break;
        case down:  flip=0; tile+=(1-f2); break;
        case left:  flip=1; tile+=(3+f2); break;
        case right: flip=0; tile+=(3+f2); break;
      }
    }
    break;
    case aNEWDWALK:
    {
      tiledir(dir);
      if ((get_bit(quest_rules,qr_BRKNSHLDTILES)) && (dummy_bool[1]==true))
      {
        tile=s_tile;
      }
      tile+=f2;
    }
    break;
    case aTEK:
    {
      if(misc==0)
      {
        tile += f2;
      }
      else if(misc!=1)
        {
          ++tile;
        }
    }
    break;
    case aNEWTEK:
    {
      if (step<0)                                             //up
      {
        switch (clk3)
        {
          case left:
          flip=0; tile+=20;
          break;
          case right:
          flip=0; tile+=24;
          break;
        }
      }
      else if (step==0)
        {
          switch (clk3)
          {
            case left:
            flip=0; tile+=8;
            break;
            case right:
            flip=0; tile+=12;
            break;
          }
        }                                                       //down
        else
        {
          switch (clk3)
          {
            case left:
            flip=0; tile+=28;
            break;
            case right:
            flip=0; tile+=32;
            break;
          }
        }
      if(misc==0)
      {
        tile+=f2;
      }
      else if(misc!=1)
        {
          tile+=2;
        }
    }
    break;
    case aARMOS:
    {
      if(misc)
      {
        tile += f2;
        if(dir==up)
          tile += 2;
      }
    }
    break;
    case aARMOS4:
    {
      switch (dir)
      {
        case up:
        flip=0;
        break;
        case down:
        flip=0; tile+=4;
        break;
        case left:
        flip=0; tile+=8;
        break;
        case right:
        flip=0; tile+=12;
        break;
      }
      if (step>1)
      {
        tile+=20;
      }
      if(misc)
      {
        tile+=f2;
      }
    }
    break;
    case aGHINI:
    {
      switch(dir)
      {
        case 8:
        case 9:
        case up: ++tile; flip=0; break;
        case 15: ++tile; flip=1; break;
        case 10:
        case 11:
        case right: flip=1; break;
        default:
        flip=0; break;
      }
    }
    break;
    case a2FRMPOS:
    {
      tile+=posframe;
    }
    break;
    case a4FRMPOS4DIR:
    {
      tiledir(dir);
      //        tile+=f2;
      tile+=posframe;
    }
    break;
    case a4FRMPOS4DIRF:
    {
      tiledir(dir);
      if (clk2>0)                                             //stopped to fire
      {
        tile+=20;
        if (clk2<17)                                          //firing
        {
          tile+=20;
        }
      }
      //        tile+=f2;
      tile+=posframe;
    }
    break;
    case a4FRMPOS8DIR:
    {
      tiledir(dir);
      //        tile+=f2;
      tile+=posframe;
    }
    break;
    case a4FRMPOS8DIRF:
    {
      tiledir(dir);
      if (clk2>0)                                             //stopped to fire
      {
        tile+=40;
        if (clk2<17)                                          //firing
        {
          tile+=40;
        }
      }
      //        tile+=f2;
      tile+=posframe;
    }
    break;
    case aNEWLEV:
    {
      tiledir(dir);
      switch(misc)
      {
        case -1:
        case 0: return;
        case 1:
//        case 5: cs = d->misc2; break;
        case 5: cs = dmisc2; break;
        case 2:
        case 4: tile += 20; break;
        case 3: tile += 40; break;
      }
      tile+=f2;
    }
    break;
    case aLEV:
    {
      f4 = ((clk/5)&1);
      switch(misc)
      {
        case -1:
        case 0: return;
        case 1:
//        case 5: tile += (f2) ? 1 : 0; cs = d->misc2; break;
        case 5: tile += (f2) ? 1 : 0; cs = dmisc2; break;
        case 2:
        case 4: tile += 2; break;
        case 3: tile += (f4) ? 4 : 3; break;
      }
    }
    break;
    case aWALLM:
    {
      if(!dummy_bool[1])
      {
        tile += f2;
      }
    }
    break;
    case aNEWWALLM:
    {
      int tempdir=0;
      switch(misc)
      {
        case 1:
        case 2: tempdir=clk3; break;
        case 3:
        case 4:
        case 5: tempdir=dir; break;
        case 6:
        case 7: tempdir=clk3^1; break;
      }
      tiledir(tempdir);
      if(!dummy_bool[1])
      {
        tile+=f2;
      }
    }
    break;
    case a4FRM3TRAP:
    {
      tiledir(dir);
      switch (family)
      {
        case eeTRAP:
        tile+=dummy_int[1]*20;
        break;
      }
      tile+=f2;
    }
    break;
    case a4FRMNODIR:
    {
      tile+=f2;
    }
    break;

  }                                                         // switch(d->anim)

  // flashing
//  if(d->flags2 & guy_flashing)
  if(flags2 & guy_flashing)
    cs = (frame&3) + 6;
}

/********************************/
/*********  Guy Class  **********/
/********************************/

// good guys, fires, fairy, and other non-enemies
// based on enemy class b/c guys in dungeons act sort of like enemies
// also easier to manage all the guys this way
guy::guy(fix X,fix Y,int Id,int Clk,bool mg) : enemy(X,Y,Id,Clk)
{
  mainguy=mg;
  canfreeze=false;
  dir=down;
  yofs=playing_field_offset;
  hxofs=2;
  hzsz=8;
  hxsz=12;
  hysz=17;
  if(!superman && !isdungeon())
    superman = 1;
}

bool guy::animate(int index)
{
  if(mainguy && clk==0 && misc==0)
  {
    setupscreen();
    misc = 1;
  }
  if(mainguy && fadeclk==0)
    return true;
  hp=256;                                                   // good guys never die...
  if(hclk && !clk2)
  {                                                         // but if they get hit...
    ++clk2;                                                 // only do this once
    if (!get_bit(quest_rules,qr_NOGUYFIRES))
    {
      addenemy(BSZ?64:72,68,eSHOOTFBALL,0);
      addenemy(BSZ?176:168,68,eSHOOTFBALL,0);
    }
  }
  return enemy::animate(index);
}

void guy::draw(BITMAP *dest)
{
  update_enemy_frame();
  if(!mainguy || fadeclk<0 || fadeclk&1)
    enemy::draw(dest);
}

/*******************************/
/*********   Enemies   *********/
/*******************************/

eFire::eFire(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //nets+1580
}

void eFire::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

void removearmos(int ax,int ay)
{
  ax&=0xF0;
  ay&=0xF0;
  int cd = (ax>>4)+ay;
  int f = MAPFLAG(ax,ay);
  int f2 = MAPCOMBOFLAG(ax,ay);

  if (combobuf[tmpscr->data[cd]].type!=cARMOS)
  {
    return;
  }

  tmpscr->data[cd] = tmpscr->undercombo;
  tmpscr->cset[cd] = tmpscr->undercset;
  tmpscr->sflag[cd] = 0;

  if(f == mfARMOS_SECRET || f2 == mfARMOS_SECRET)
  {
    tmpscr->data[cd] = tmpscr->secretcombo[sSTAIRS];
    tmpscr->cset[cd] = tmpscr->secretcset[sSTAIRS];
    tmpscr->sflag[cd]=tmpscr->secretflag[sSTAIRS];
    if (!nosecretsounds)
    {
      sfx(WAV_SECRET);
    }
  }

  if(f == mfARMOS_ITEM || f2 == mfARMOS_ITEM)
  {
    if(!getmapflag())
    {
      additem(ax,ay,tmpscr->catchall, ipONETIME + ipBIGRANGE
              | ((tmpscr->flags3&fHOLDITEM) ? ipHOLDUP : 0)
             );
      if (!nosecretsounds)
      {
        sfx(WAV_SECRET);
      }
    }
  }

  putcombo(scrollbuf,ax,ay,tmpscr->data[cd],tmpscr->cset[cd]);
}

eArmos::eArmos(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,0)
{
  //these are here to bypass compiler warnings about unused arguments
  Clk=Clk;

  //nets+680;
  //    tile = d->tile + (rand()&1); // wth?
  fromstatue=true;
  superman = 1;
  fading=fade_flicker;
//  step=(rand()&1)?(d->misc1)/100.0:(d->step)/100.0;
  step=(rand()&1)?(dmisc1)/100.0:dstep/100.0;
  count_enemy=false;
  if(!canmove(down,(fix)8,spw_none))
    dir=-1;
  else
  {
    dir=down;
    clk3=int(13.0/step);
  }
}

bool eArmos::animate(int index)
{
  if(dying)
    return Dead();
  if(misc)
  {
//    constant_walk(d->rate, d->homing, 0);
    constant_walk(rate, homing, 0);
  }
  else if(++clk2 > 60)
  {
    misc=1;
    superman=0;
    fading=0;
    removearmos(x,y);
    clk=-1;
    clk2=0;
    if(dir==-1)
    {
      dir=0;
      y.v&=0xF00000;
    }
  }

  return enemy::animate(index);
}

void eArmos::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eGhini::eGhini(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (dmisc1/*id==eGHINI2*/&&get_bit(quest_rules,qr_PHANTOMGHINI2))
  {
    drawstyle=1;
  }
  //nets+620;
//  if(d->misc1)
  if(dmisc1)
  {
    //fixing      step=0;
    fading=fade_flicker;
    count_enemy=false;
    dir=12;
    movestatus=1;
    clk=0;
  }
  //frate=256;                                                // don't use frate, allow clk to go up past 160
}

bool eGhini::animate(int index)
{
  if(dying)
    return Dead();
//  if(d->misc1==0 && clk>=0)
  if(dmisc1==0 && clk>=0)
  {
//    constant_walk(d->rate,d->homing,0);
    constant_walk(rate,homing,0);
  }
//  if(d->misc1)
  if(dmisc1)
  {
    if(misc)
    {
      if(clk>160)
        misc=2;
      floater_walk((misc==1)?0:4,12,(fix)0.625,(fix)0.0625,10,120,10);
    }
    else if(clk>=60)
      {
        misc=1;
        clk3=32;
        fading=0;
        guygrid[(int(y)&0xF0)+(int(x)>>4)]=0;
      }
  }

  return enemy::animate(index);
}

void eGhini::draw(BITMAP *dest)
{
  if (dmisc1/*(id==eGHINI2)*/&&(get_bit(quest_rules,qr_GHINI2BLINK))&&(clk&1))
  {
    return;
  }
  update_enemy_frame();
  enemy::draw(dest);
}

void eGhini::kickbucket()
{
  hp=-1000;                                                 // don't call death_sfx()
}

eTektite::eTektite(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dir=down;
  misc=1;
  clk=-15;
  oldy=y;
  if(!BSZ)
    clk*=rand()%3+1;

  // avoid divide by 0 errors
  if(dmisc1 == 0)
    dmisc1 = 24;
  if(dmisc2 == 0)
    dmisc2 = 3;
  //nets+760;
}

bool eTektite::animate(int index)
{
  if(dying)
    return Dead();
  if(get_bit(quest_rules,qr_ENEMIESZAXIS))
    y=oldy;
  if(clk>=0 && !stunclk && (!watch || misc==0))
    switch(misc)
    {
      case 0:                                               // normal
      if(!(rand()%dmisc1))
      {
        misc=1;
        clk2=32;
      }
      break;

      case 1:                                               // waiting to pounce
      if(--clk2<=0)
      {
        int r=rand();
        misc=2;
        step=0-((dstep)/100.0);                           // initial speed
        clk3=(r&1)+2;                                       // left or right
        clk2start=clk2=(r&31)+10;                           // flight time
        if(y<32)  clk2+=2;                                  // make them come down from top of screen
        if(y>112) clk2-=2;                                  // make them go back up
        cstart=c = 9-((r&31)>>3);                           // time before gravity kicks in
      }
      break;

      case 2:                                                 // in flight
      move(step);
      if (step>0)                                           //going down
      {
        if (COMBOTYPE(x+8,y+16)==cNOJUMPZONE)
        {
          step=0-step;
        }
		else if (COMBOTYPE(x+8,y+16)==cNOENEMY)
        {
          step=0-step;
        }
		else if (MAPFLAG(x+8,y+16)==mfNOENEMY)
        {
          step=0-step;
        }
		else if (MAPCOMBOFLAG(x+8,y+16)==mfNOENEMY)
        {
          step=0-step;
        }
      }
      else if (step<0)
        {
          if (COMBOTYPE(x+8,y)==cNOJUMPZONE)
          {
            step=0-step;
          }
		  else if (COMBOTYPE(x+8,y)==cNOENEMY)
          {
            step=0-step;
          }
		  else if (MAPFLAG(x+8,y)==mfNOENEMY)
          {
            step=0-step;
          }
		  else if (MAPCOMBOFLAG(x+8,y)==mfNOENEMY)
          {
            step=0-step;
          }
        }

        if (clk3==left)
      {
        if (COMBOTYPE(x,y+8)==cNOJUMPZONE)
        {
          clk3^=1;
        }
		else if (COMBOTYPE(x,y+8)==cNOENEMY)
        {
          clk3^=1;
        }
		else if (MAPFLAG(x,y+8)==mfNOENEMY)
        {
          clk3^=1;
        }
		else if (MAPCOMBOFLAG(x,y+8)==mfNOENEMY)
        {
          clk3^=1;
        }
      }
      else
      {
        if (COMBOTYPE(x+16,y+8)==cNOJUMPZONE)
        {
          clk3^=1;
        }
		else if (COMBOTYPE(x+16,y+8)==cNOENEMY)
        {
          clk3^=1;
        }
		else if (MAPFLAG(x+16,y+8)==mfNOENEMY)
        {
          clk3^=1;
        }
		else if (MAPCOMBOFLAG(x+16,y+8)==mfNOENEMY)
        {
          clk3^=1;
        }
      }
      --c;
      if(c<0 && step<(dstep/100.0))
      {
        step+=(dmisc3/100.0);
      }
	  int nb=get_bit(quest_rules,qr_NOBORDER) ? 16 : 0;
      if(x<=16-nb)  clk3=right;
      if(x>=224+nb) clk3=left;
      x += (clk3==left) ? -1 : 1;
      if((--clk2<=0 && y>=16-nb) || y>=144+nb)
      {
        if(rand()%dmisc2)                                 //land and wait
        {
          clk=misc=0;
        }                                                   //land and jump again
        else
        {
          misc=1;
          clk2=0;
        }
      }
      break;

    }                                                         // switch

  if(get_bit(quest_rules,qr_ENEMIESZAXIS) && misc==2)
  {
    int tempy=oldy;
    z=zc_max(0,zc_min(clk2start-clk2,clk2));
    oldy=y;
    y=tempy-z;
  }
  if(stunclk && (clk&31)==1)
    clk=0;
  return enemy::animate(index);
}

void eTektite::drawshadow(BITMAP *dest,bool translucent)
{
  int tempy=yofs;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if(misc==0)
    {
      shadowtile+=f2;
    }
    else if(misc!=1)
        shadowtile+=2;
  }
  else
  {
    if(misc==0)
    {
      shadowtile += f2 ? 1 : 0;
    }
    else if(misc!=1)
      {
        ++shadowtile;
      }
  }
  yofs+=8;
  if (!get_bit(quest_rules,qr_ENEMIESZAXIS) && misc==2)
  {
    yofs+=zc_max(0,zc_min(clk2start-clk2,clk2));
  }
  enemy::drawshadow(dest,translucent);
  yofs=tempy;
}

void eTektite::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eItemFairy::eItemFairy(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  step=(fix)(guysbuf[id&0xFFF].step)/100;
  superman=1;
  dir=8;
  hxofs=1000;
  mainguy=false;
  count_enemy=false;
}

bool eItemFairy::animate(int index)
{
  if(dying)
    return Dead();
  //if(clk>32)
    misc=1;
  bool w=watch;
  watch=false;
  variable_walk_8(misc?3:0,8,spw_floater);
  watch=w;
  return enemy::animate(index);
}

void eItemFairy::draw(BITMAP *dest)
{
  //these are here to bypass compiler warnings about unused arguments
  dest=dest;
}

ePeahat::ePeahat(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{//floater_walk(int rate,int newclk,fix ms,fix ss,int s,int p, int g)
  floater_walk(misc?4:0,      8, (fix)0.625, (fix)0.0625, 10,  80, 16);
  dir=8;
  movestatus=1;
  clk=0;
  //nets+720;
}

bool ePeahat::animate(int index)
{
  if(dying)
    return Dead();
  if(stunclk==0 && clk>96)
    misc=1;
  floater_walk(misc?4:0,      8, (fix)0.625, (fix)0.0625, 10,  80, 16);
  if (get_bit(quest_rules,qr_ENEMIESZAXIS) && !(tmpscr->flags7&fSIDEVIEW))
  {
    z=int(step*1.1/(fix)0.0875);
  }
  superman = (movestatus && !get_bit(quest_rules,qr_ENEMIESZAXIS)) ? ((flags&guy_sbombonly) ? 2 : 1) : 0;
  stunclk=0;
  return enemy::animate(index);
}

void ePeahat::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile+posframe;
  if (!get_bit(quest_rules,qr_ENEMIESZAXIS))
  {
    yofs+=8;
    yofs+=int(step/(fix)0.0625);
  }
  enemy::drawshadow(dest,translucent);
  yofs=tempy;
}

void ePeahat::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

int ePeahat::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;

  if(dying || clk<0 || hclk>0)
    return 0;
  if(superman && !(wpnId==wSBomb && superman==2)            // vulnerable to super bombs
                                                            // fire boomerang, for nailing peahats
     && !(wpnId==wBrang && current_item_power(itype_brang)>0))
    return 0;


  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    return 0;
    case wBrang:
//    if(!(d->flags & guy_bhit))
    if(!(flags & guy_bhit))
    {
      //if (current_item(itype_brang,true)==3)
      {
        stunclk=160;
        clk2=0;
        movestatus=0;
        misc=0;
        clk=0;
        step=0;
        //            floater_walk(misc?4:0,8,0.625,0.0625,10,240,16);

      }
      break;
    }
    default:
    hp-=power;
    hclk=33;
    if((dir&2)==(wpnDir&2))
      sclk=(wpnDir<<8)+16;
  }
  //    if(wpnId==wBrang && hp<=0)
  if(((wpnId==wBrang) || (get_bit(quest_rules,qr_NOFLASHDEATH))) && hp<=0)
  {
    fading=fade_blue_poof;
  }
  sfx(WAV_EHIT,pan(int(x)));
  return 1;
}

eLeever::eLeever(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
//  if(d->misc1==0) { misc=-1; clk-=16; } //Line of Sight leevers
  if(dmisc1==0) { misc=-1; clk-=16; } //Line of Sight leevers
                                        //nets+1460;
  temprule=(get_bit(quest_rules,qr_NEWENEMYTILES)) != 0;
}

bool eLeever::animate(int index)
{
  if(dying)
    return Dead();
  if(clk>=0 && !slide())
  {
//    switch(d->misc1)
    switch(dmisc1)
    {
      case 0:      //line of sight
	  case 2:
      switch(misc) //is this leever active
      {
        case -1:  //submerged
        {
          if ((dmisc1==2)&&(rand()&255))
          {
            break;
          }
          int active=0;
          for(int i=0; i<guys.Count(); i++)
          {
            if(guys.spr(i)->id==id && (((enemy*)guys.spr(i))->misc>=0))
            {
              ++active;
            }
          }
          if(active<((dmisc1==2)?1:2))
          {
            misc=0; //activate this one
          }
        }
        break;

        case 0:
        {
          int s=0;
          for(int i=0; i<guys.Count(); i++)
          {
            if(guys.spr(i)->id==id && ((enemy*)guys.spr(i))->misc==1)
            {
              ++s;
            }
          }
          if(s>0)
          {
            break;
          }
          int d2=rand()&1;
          if(LinkDir()>=left)
          {
            d2+=2;
          }
          if(canplace(d2) || canplace(d2^1))
          {
            misc=1;
            clk2=0;
            clk=0;
          }
        }
        break;

        case 1: if(++clk2>16) misc=2; break;
        case 2: if(++clk2>24) misc=3; break;
//        case 3: if(stunclk) break; if(scored) dir^=1; if(!canmove(dir)) misc=4; else move((fix)(d->step/100.0)); break;
        case 3: if(stunclk) break; if(scored) dir^=1; if(!canmove(dir)) misc=4; else move((fix)(dstep/100.0)); break;
        case 4: if(--clk2==16) { misc=5; clk=8; }
        break;
        case 5: if(--clk2==0)  misc=((dmisc1==2)?-1:0); break;
      }                                                       // switch(misc)
      break;

      default:  //random
//      step=d->misc3/100.0;
      step=dmisc3/100.0;
      ++clk2;
      if(clk2<32)    misc=1;
      else if(clk2<48)    misc=2;
//      else if(clk2<300) { misc=3; step = d->step/100.0; }
      else if(clk2<300) { misc=3; step = dstep/100.0; }
      else if(clk2<316)   misc=2;
      else if(clk2<412)   misc=1;
      else if(clk2<540) { misc=0; step=0; }
      else clk2=0;
      if(clk2==48) clk=0;
//      variable_walk(d->rate, d->homing, 0);
      variable_walk(rate, homing, 0);
    }                                                         // switch(dmisc1)
  }

  hxofs=(misc>=2)?0:1000;
  return enemy::animate(index);
}

bool eLeever::canplace(int d2)
{
  int nx=LinkX();
  int ny=LinkY();

  if(d2<left) ny&=0xF0;
  else       nx&=0xF0;

  switch(d2)
  {
//    case up:    ny-=((d->misc1==0)?32:48); break;
//    case down:  ny+=((d->misc1==0)?32:48); if(ny-LinkY()<32) ny+=((d->misc1==0)?16:0); break;
//    case left:  nx-=((d->misc1==0)?32:48); break;
//    case right: nx+=((d->misc1==0)?32:48); if(nx-LinkX()<32) nx+=((d->misc1==0)?16:0); break;
    case up:    ny-=((dmisc1==0||dmisc1==2)?32:48); break;
    case down:  ny+=((dmisc1==0||dmisc1==2)?32:48); if(ny-LinkY()<32) ny+=((dmisc1==0||dmisc1==2)?16:0); break;
    case left:  nx-=((dmisc1==0||dmisc1==2)?32:48); break;
    case right: nx+=((dmisc1==0||dmisc1==2)?32:48); if(nx-LinkX()<32) nx+=((dmisc1==0||dmisc1==2)?16:0); break;
  }
  if(m_walkflag(nx,ny,spw_halfstep ))                       /*none*/
    return false;
  x=nx;
  y=ny;
  dir=d2^1;
  return true;
}

void eLeever::draw(BITMAP *dest)
{
//  cs=d->cset;
  cs=dcset;
  update_enemy_frame();
  switch(misc)
  {
    case -1:
    case 0: return;
  }
  enemy::draw(dest);
}

int eLeever::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if (wpnId==wStomp)
  {
    sfx(WAV_CHINK);
    return 1;
  }
  return enemy::takehit(wpnId,power,wpnx,wpny,wpnDir);

}

eGel::eGel(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //nets+200;
  clk2=-1;
/*
  if(id>=0x1000)
    count_enemy=false;
*/
  hxsz=hysz=14;
}

bool eGel::animate(int index)
{
  if(dying)
  {
    return Dead();
  }
  if(clk>=0)
  {
    switch(id>>12)
    {
      case 0:
//        halting_walk(d->rate, d->homing, spw_none, d->hrate, ((rand()&7)<<3)+2);
        halting_walk(rate, homing, spw_none, hrate, ((rand()&7)<<3)+2);
        if(dmisc1==1/*(id&0xFFF)!=eGEL*/&&clk2==1&&wpn)
        {
//          addEwpn(x,y,z,d->weapon,0,d->wdp,-1);
          addEwpn(x,y,z,wpn,0,wdp,-1,getUID());
          sfx(WAV_FIRE,pan(int(x)));

          int i=Ewpns.Count()-1;
          weapon *ew = (weapon*)(Ewpns.spr(i));
          if (wpnsbuf[wpn].frames>1)
          {
            ew->aframe=rand()%wpnsbuf[wpn].frames;
            ew->tile+=ew->aframe;
          }
        }
        break;
      case 1:
        if(misc==1)
        {
          dir=up;
          step=8;
        }
        if(misc<=2)
        {
          move(step);
          if(!canmove(dir,(fix)0,0))
            dir=down;
        }
        if(misc==3)
        {
          if(canmove(right,(fix)16,0))
            x+=16;
        }
        ++misc;
        break;
      case 2:
        if(misc==1)
        {
          dir=down;
          step=8;
        }
        if(misc<=2)
          move(step);
        if(misc==3)
        {
          if(canmove(left,(fix)16,0))
            x-=16;
        }
        ++misc;
        break;
    }
  }
  if(misc>=4)
  {
    id&=0xFFF;
//    step = d->step/100.0;
    step = dstep/100.0;
    if(x<32) x=32;
    if(x>208) x=208;
    if(y<32) y=32;
    if(y>128) y=128;
  }

  return enemy::animate(index);
}

void eGel::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eZol::eZol(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk2=-1;
  //nets+220;
}

bool eZol::animate(int index)
{
  if(dying)
    return Dead();
//  if(hp>0 && hp<guysbuf[eGEL+1].hp && !slide())
  if(hp>0 && hp<guysbuf[id].hp && !slide())
  {
    int kids = guys.Count();
    int id2=dmisc2;//(id==eFZOL?eFGEL:eGEL);
	for(int i=0;i<dmisc3;i++)
	{
	  if(addenemy(x,y,id2+((i+1)<<12),-22))
		((enemy*)guys.spr(kids+i))->count_enemy = false;
	}

    //guys.add(new eGel(x,y,id2+0x1000,-21));
    //guys.add(new eGel(x,y,id2+0x2000,-22));
    //((enemy*)guys.spr(kids))->count_enemy = false;
    //((enemy*)guys.spr(kids+1))->count_enemy = false;
    return true;
  }
  else
  {
//    halting_walk(d->rate, d->homing, spw_none, d->hrate, (rand()&7)<<4);
    halting_walk(rate, homing, spw_none, hrate, (rand()&7)<<4);
    if(dmisc1==1/*id==eFZOL*/&&clk2==1&&wpn)
    {
//      addEwpn(x,y,z,d->weapon,0,d->wdp,-1);
      addEwpn(x,y,z,wpn,0,wdp,dir,getUID());
      sfx(WAV_FIRE,pan(int(x)));

      int i=Ewpns.Count()-1;
      weapon *ew = (weapon*)(Ewpns.spr(i));
      if (wpnsbuf[ewFIRETRAIL].frames>1)
      {
        ew->aframe=rand()%wpnsbuf[ewFIRETRAIL].frames;
        ew->tile+=ew->aframe;
      }
    }
  }

  return enemy::animate(index);
}

void eZol::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

int eZol::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  int ret = enemy::takehit(wpnId,power,wpnx,wpny,wpnDir);
  if(sclk)
    sclk+=128;
  return ret;
}

eGelTrib::eGelTrib(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk4=0;
  clk2=-1;
/*
  if(id>=0x1000)
    count_enemy=false;
*/
  hxsz=hysz=14;
  //nets+240;
}

bool eGelTrib::animate(int index)
{
  ++clk4;
  if(dying)
  {
    return Dead();
  }
  if(clk>=0)
  {
    switch(id>>12)
    {
      case 0:
//        halting_walk(d->rate, d->homing, spw_none, d->hrate, ((rand()&7)<<3)+2);
        halting_walk(rate, homing, spw_none, hrate, ((rand()&7)<<3)+2);
        if(dmisc1==1/*(id&0xFFF)!=eGELTRIB*/&&clk2==1&&wpn)
        {
          addEwpn(x,y,z,wpn,0,4*DAMAGE_MULTIPLIER,dir,getUID());
          sfx(WAV_FIRE,pan(int(x)));

          int i=Ewpns.Count()-1;
          weapon *ew = (weapon*)(Ewpns.spr(i));
          if (wpnsbuf[ewFIRETRAIL].frames>1)
          {
            ew->aframe=rand()%wpnsbuf[ewFIRETRAIL].frames;
            ew->tile+=ew->aframe;
          }
        }
        break;
      case 1:
        if(misc==1)
        {
          dir=up;
          step=8;
        }
        if(misc<=2)
        {
          move(step);
          if(!canmove(dir,(fix)0,0))
            dir=down;
        }
        if(misc==3)
        {
          if(canmove(right,(fix)16,0))
            x+=16;
        }
        ++misc;
        break;
      case 2:
        if(misc==1)
        {
          dir=down;
          step=8;
        }
        if(misc<=2)
          move(step);
        if(misc==3)
        {
          if(canmove(left,(fix)16,0))
            x-=16;
        }
        ++misc;
        break;
    }
  }
  if(misc>=4)
  {
    id&=0xFFF;
//    step = d->step/100.0;
    step = dstep/100.0;
    if(x<32) x=32;
    if(x>208) x=208;
    if(y<32) y=32;
    if(y>128) y=128;
  }
  if (clk4==256)
  {
    int kids = guys.Count();
	bool success = false;
	if(get_bit(quest_rules, qr_OLDTRIBBLES))
	{
		int id2=dmisc2;//(id==eFGELTRIB?eFZOL:eZOL);
		success = addenemy((fix)x,(fix)y,id2,-21);
	}
	else
	{
		int id2=dmisc3;//(id==eFGELTRIB?eFZOLTRIB:eZOLTRIB);
		success = addenemy((fix)x,(fix)y,id2,-21);
	}
	if(success)
		((enemy*)guys.spr(kids))->count_enemy = count_enemy;
    return true;
  }

  return enemy::animate(index);
}

void eGelTrib::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eZolTrib::eZolTrib(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk2=-1;
  //nets+260;
}

bool eZolTrib::animate(int index)
{
  if(dying)
    return Dead();
  if(hp>0 && hp<guysbuf[id].hp && !slide())
  {
    int kids = guys.Count();
	int id2=dmisc2;//(id==eFZOL?eFGEL:eGEL);
	for(int i=0;i<dmisc3;i++)
	{
	  if(addenemy(x,y,id2+((i+1)<<12),-22))
		((enemy*)guys.spr(kids+i))->count_enemy = false;
	}
    return true;
  }
  else
  {
    halting_walk(rate, homing, spw_none, hrate, (rand()&7)<<4);
    if(dmisc1==1/*id==eFZOLTRIB*/&&clk2==1&&wpn)
    {
      addEwpn(x,y,z,wpn,0,4*DAMAGE_MULTIPLIER,dir,getUID());
      sfx(WAV_FIRE,pan(int(x)));

      int i=Ewpns.Count()-1;
      weapon *ew = (weapon*)(Ewpns.spr(i));
      if (wpnsbuf[ewFIRETRAIL].frames>1)
      {
        ew->aframe=rand()%wpnsbuf[ewFIRETRAIL].frames;
        ew->tile+=ew->aframe;
      }
    }
  }

  return enemy::animate(index);
}

void eZolTrib::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

int eZolTrib::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  int ret = enemy::takehit(wpnId,power,wpnx,wpny,wpnDir);
  if(sclk)
    sclk+=128;
  return ret;
}

eWallM::eWallM(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  haslink=false;
  //nets+1000;
}

bool eWallM::animate(int index)
{
  if(dying)
    return Dead();
  hxofs=1000;
  if(misc==0) //inside wall, ready to spawn?
  {
    if(frame-wallm_load_clk>80 && clk>=0)
    {
      int wall=link_on_wall();
      int wallm_cnt=0;
      for(int i=0; i<guys.Count(); i++)
        if(((enemy*)guys.spr(i))->family==eeWALLM)
        {
          register int m=((enemy*)guys.spr(i))->misc;
          if(m && ((enemy*)guys.spr(i))->clk3==(wall^1))
          {
            ++wallm_cnt;
          }
        }
        if(wall>0)
      {
        --wall;
        misc=1; //emerging from the wall?
        clk2=0;
        clk3=wall^1;
        wallm_load_clk=frame;
        if(wall<=down)
        {
          if(LinkDir()==left)
            dir=right;
          else
            dir=left;
        }
        else
        {
          if(LinkDir()==up)
            dir=down;
          else
            dir=up;
        }
        switch(wall)
        {
          case up:    y=0;   break;
          case down:  y=160; break;
          case left:  x=0;   break;
          case right: x=240; break;
        }
        switch(dir)
        {
          case up:    y=LinkY()+48-(wallm_cnt&1)*12; flip=wall&1;           break;
          case down:  y=LinkY()-48+(wallm_cnt&1)*12; flip=((wall&1)^1)+2;   break;
          case left:  x=LinkX()+48-(wallm_cnt&1)*12; flip=(wall==up?2:0)+1; break;
          case right: x=LinkX()-48+(wallm_cnt&1)*12; flip=(wall==up?2:0);   break;
        }
      }
    }
  }
  else
    wallm_crawl();
  return enemy::animate(index);
}

void eWallM::wallm_crawl()
{
  bool w=watch;
  hxofs=0;
  if(slide())
  {
    return;
  }
  //  if(dying || watch || (!haslink && stunclk))
  if(dying || (!haslink && stunclk))
  {
    return;
  }

  watch=false;
  ++clk2;
  misc=(clk2/40)+1;
  if (w&&misc>=3&&misc<=5)
  {
    --clk2;
  }
  switch(misc)
  {
    case 1:
    case 2: zc_swap(dir,clk3); move(step); zc_swap(dir,clk3); break;
    case 3:
    case 4:
    case 5: if (w) {watch=w; return; } move(step); break;
    case 6:
    case 7: zc_swap(dir,clk3); dir^=1; move(step); dir^=1; zc_swap(dir,clk3); break;
    default: misc=0; break;
  }
  watch=w;
}

void eWallM::grablink()
{
  haslink=true;
  superman=1;
}

void eWallM::draw(BITMAP *dest)
{
  dummy_bool[1]=haslink;
  update_enemy_frame();
  if(misc>0)
  {
    masked_draw(dest,16,playing_field_offset+16,224,144);
  }
  //    enemy::draw(dest);
  //    tile = clk&8 ? 128:129;
}

eTrap::eTrap(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  ox=x;                                                     //original x
  oy=y;                                                     //original y
  if (get_bit(quest_rules,qr_TRAPPOSFIX))
  {
    yofs = playing_field_offset;
  }
  mainguy=false;
  count_enemy=false;
  //nets+420;
  dummy_int[1]=0;
}

bool eTrap::animate(int index)
{
  if(clk<0)
    return enemy::animate(index);

  if(misc==0)                                               // waiting
  {
    double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
    if ((ddir<=(((-1)*PI)/4))&&(ddir>(((-3)*PI)/4)))
    {
      dir=down;
    }
    else if ((ddir<=(((1)*PI)/4))&&(ddir>(((-1)*PI)/4)))
    {
      dir=right;
    }
    else if ((ddir<=(((3)*PI)/4))&&(ddir>(((1)*PI)/4)))
    {
      dir=up;
    }
    else
    {
      dir=left;
    }
    int d2=lined_up(15,true);
    if (((d2<left || d2 > right) && (dmisc1==1)) ||
          ((d2>down) && (dmisc1==2)) ||
          ((d2>right) && (!dmisc1)) ||
          ((d2<l_up) && (dmisc1==4)) ||
          ((d2!=r_up) && (d2!=l_down) && (dmisc1==6)) ||
          ((d2!=l_up) && (d2!=r_down) && (dmisc1==8)))
    {
      d2=-1;
    }
    if(d2!=-1 && trapmove(d2))
    {
      dir=d2;
      misc=1;
      clk2=(dir==down)?3:0;
    }
  }

  if(misc==1)                                               // charging
  {
    clk2=(clk2+1)&3;
    step=(clk2==3)?1:2;
    if(!trapmove(dir) || clip())
    {
      misc=2;
      if (dir<l_up)
      {
        dir=dir^1;
      }
      else
      {
        dir=dir^3;
      }
    }
    else
    {
      sprite::move(step);
    }
  }
  if(misc==2)                                               // retreating
  {
    step=(++clk2&1)?1:0;
    switch (dir)
    {
      case up:
      case left:
      case l_up:
        if(int(x)<=ox && int(y)<=oy)
        {
          x=ox;
          y=oy;
          misc=0;
        }
        else
        {
          sprite::move(step);
        }
        break;
      case down:
      case right:
      case r_down:
        if(int(x)>=ox && int(y)>=oy)
        {
          x=ox;
          y=oy;
          misc=0;
        }
        else
        {
          sprite::move(step);
        }
        break;
      case r_up:
        if(int(x)>=ox && int(y)<=oy)
        {
          x=ox;
          y=oy;
          misc=0;
        }
        else
        {
          sprite::move(step);
        }
        break;
      case l_down:
        if(int(x)<=ox && int(y)>=oy)
        {
          x=ox;
          y=oy;
          misc=0;
        }
        else
        {
          sprite::move(step);
        }
        break;
    }
  }

  return enemy::animate(index);
}

bool eTrap::trapmove(int ndir)
{
  if(get_bit(quest_rules,qr_MEANTRAPS))
  {
    if(tmpscr->flags2&fFLOATTRAPS)
      return canmove(ndir,(fix)1,spw_floater, 0, 0, 15, 15);
    return canmove(ndir,(fix)1,spw_water, 0, 0, 15, 15);
  }
  if(oy==80 && ndir<left)
    return false;
  if(oy<80 && ndir==up)
    return false;
  if(oy>80 && ndir==down)
    return false;
  if(ox<128 && ndir==left)
    return false;
  if(ox>128 && ndir==right)
    return false;
  if(ox<128 && oy<80 && ndir==l_up)
    return false;
  if(ox<128 && oy>80 && ndir==l_down)
    return false;
  if(ox>128 && oy<80 && ndir==r_up)
    return false;
  if(ox>128 && oy>80 && ndir==r_down)
    return false;
  return true;
}

bool eTrap::clip()
{
  if(get_bit(quest_rules,qr_MEANPLACEDTRAPS))
  {
    switch(dir)
    {
      case up:      if (y<=0)           return true; break;
      case down:    if (y>=160)         return true; break;
      case left:    if (x<=0)           return true; break;
      case right:   if (x>=240)         return true; break;
      case l_up:    if (y<=0||x<=0)     return true; break;
      case l_down:  if (y>=160||x<=0)   return true; break;
      case r_up:    if (y<=0||x>=240)   return true; break;
      case r_down:  if (y>=160||x>=240) return true; break;
    }
    return false;
  }
  else
  {
    switch(dir)
    {
      case up:      if(oy>80 && y<=86) return true; break;
      case down:    if(oy<80 && y>=80) return true; break;
      case left:    if(ox>128 && x<=124) return true; break;
      case right:   if(ox<120 && x>=116) return true; break;
      case l_up:    if(oy>80 && y<=86 && ox>128 && x<=124) return true; break;
      case l_down:  if(oy<80 && y>=80 && ox>128 && x<=124) return true; break;
      case r_up:    if(oy>80 && y<=86 && ox<120 && x>=116) return true; break;
      case r_down:  if(oy<80 && y>=80 && ox<120 && x>=116) return true; break;
    }
    return false;
  }
}

void eTrap::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

int eTrap::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnId=wpnId;
  power=power;
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  return 0;
}

eTrap2::eTrap2(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  lasthit=-1;
  lasthitclk=0;
  mainguy=false;
  count_enemy=false;
  step=2;

  if (!dmisc1)
  {
    dir=(x<=112)?right:left;
  }
  if (dmisc1==1)
  {
    dir=(y<=72)?down:up;
  }
  if (get_bit(quest_rules,qr_TRAPPOSFIX))
  {
    yofs = playing_field_offset;
  }
  //nets+((id==eTRAP_LR)?540:520);
}

bool eTrap2::animate(int index)
{
  if(clk<0)

    return enemy::animate(index);

  if (!get_bit(quest_rules,qr_PHANTOMPLACEDTRAPS))
  {
    if (lasthitclk>0)
    {
      --lasthitclk;
    }
    else
    {
      lasthit=-1;
    }
    bool hitenemy=false;
    for(int j=0; j<guys.Count(); j++)
    {
      if ((j!=index) && (lasthit!=j))
      {
        if(hit(guys.spr(j)))
        {
          lasthit=j;
          lasthitclk=10;
          hitenemy=true;
          guys.spr(j)->lasthit=index;
          guys.spr(j)->lasthitclk=10;
//          guys.spr(j)->dir=guys.spr(j)->dir^1;
        }
      }
    }
    if(!trapmove(dir) || clip() || hitenemy)
    {
      if(!trapmove(dir) || clip())
      {
        lasthit=-1;
        lasthitclk=0;
      }
      if(get_bit(quest_rules,qr_MORESOUNDS))
        sfx(WAV_ZN1TAP,pan(int(x)));
      dir=dir^1;
    }
    sprite::move(step);
  }
  else
  {
    if(!trapmove(dir) || clip())
    {
      if(get_bit(quest_rules,qr_MORESOUNDS))
        sfx(WAV_ZN1TAP,pan(int(x)));
      dir=dir^1;
    }
    sprite::move(step);
  }
  return enemy::animate(index);
}

bool eTrap2::trapmove(int ndir)
{
  if(tmpscr->flags2&fFLOATTRAPS)
    return canmove(ndir,(fix)1,spw_floater, 0, 0, 15, 15);
  return canmove(ndir,(fix)1,spw_water, 0, 0, 15, 15);
}

bool eTrap2::clip()
{
  switch(dir)
  {
    case up:    if(y<=0) return true; break;
    case down:  if(y>=160) return true; break;
    case left:  if(x<=0) return true; break;
    case right: if(x>=240) return true; break;
  }
  return false;
}

void eTrap2::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

int eTrap2::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnId=wpnId;
  power=power;
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  return 0;
}

eRock::eRock(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //do not show "enemy appering" anim -DD
  clk=0;
  mainguy=false;
  clk2=-14;
  hxofs=hyofs=-2;
  hxsz=hysz=20;
  //nets+1640;
}

bool eRock::animate(int index)
{
  if(dying)
    return Dead();
  if(++clk2==0)                                             // start it
  {
    x=rand()&0xF0;
    y=0;
    clk3=0;
    clk2=rand()&15;
  }
  if(clk2>16)                                               // move it
  {
    if(clk3<=0)                                             // start bounce
    {
      dir=rand()&1;
      if(x<32)  dir=1;
      if(x>208) dir=0;
    }
    if(clk3<13+16)
    {
      x += dir ? 1 : -1;                                    //right, left
      dummy_int[1]=dir;
      if(clk3<2)
      {
        y-=2;    //up
        dummy_int[2]=(dummy_int[1]==1)?r_up:l_up;
      }
      else if(clk3<5)
        {
          y--;    //up
          dummy_int[2]=(dummy_int[1]==1)?r_up:l_up;
        }
        else if(clk3<8)
          {
            dummy_int[2]=(dummy_int[1]==1)?right:left;
          }
          else if(clk3<11)
            {
              y++;   //down
              dummy_int[2]=(dummy_int[1]==1)?r_down:l_down;
            }
            else
            {
              y+=2; //down
              dummy_int[2]=(dummy_int[1]==1)?r_down:l_down;
            }

      ++clk3;
    }
    else if(y<176)
        clk3=0;                                               // next bounce
      else
        clk2 = -(rand()&63);                                  // back to top
  }
  return enemy::animate(index);
}

void eRock::drawshadow(BITMAP *dest, bool translucent)
{
  if(clk2>=0)
  {
	  int tempy=yofs;
	  flip = 0;
	  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
		(clk/(frate/4)):((clk>=(frate>>1))?1:0);
	  shadowtile = wpnsbuf[iwShadow].tile+f2;

	  yofs+=8;
	  yofs+=zc_max(0,zc_min(29-clk3,clk3));
	  enemy::drawshadow(dest, translucent);
	  yofs=tempy;
  }
}

void eRock::draw(BITMAP *dest)
{
  if(clk2>=0)
  {
    int tempdir=dir;
    dir=dummy_int[2];
    update_enemy_frame();
    enemy::draw(dest);
    dir=tempdir;
  }
}

int eRock::takehit(int, int, int, int, int)
{
  return 0;
}

eBoulder::eBoulder(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk=0;
  mainguy=false;
  clk2=-14;
  hxofs=hyofs=-10;
  hxsz=hysz=36;
  hzsz=16; //can't be jumped
  //nets+1680;
}

bool eBoulder::animate(int index)
{
  if(dying)
    return Dead();
  fix *vert;
  vert = get_bit(quest_rules,qr_ENEMIESZAXIS) ? &z : &y;
  if(++clk2==0)                                             // start it
  {
    x=rand()&0xF0;
    y=-32;
    clk3=0;
    clk2=rand()&15;
  }
  if(clk2>16)                                               // move it
  {
    if(clk3<=0)                                             // start bounce
    {
      dir=rand()&1;
      if(x<32)  dir=1;
      if(x>208) dir=0;
    }
    if(clk3<13+16)
    {
      x += dir ? 1 : -1;                                    //right, left
      dummy_int[1]=dir;
      if(clk3<2)
      {
        y-=2;    //up
        dummy_int[2]=(dummy_int[1]==1)?r_up:l_up;
      }
      else if(clk3<5)
        {
          y--;    //up
          dummy_int[2]=(dummy_int[1]==1)?r_up:l_up;
        }
        else if(clk3<8)
          {
            dummy_int[2]=(dummy_int[1]==1)?right:left;
          }
          else if(clk3<11)
            {
              y++;     //down
              dummy_int[2]=(dummy_int[1]==1)?r_down:l_down;
            }
            else
            {
              y+=2; //down
              dummy_int[2]=(dummy_int[1]==1)?r_down:l_down;
            }

      ++clk3;
    }
    else if(y<176)
        clk3=0;                                               // next bounce
      else
        clk2 = -(rand()&63);                                  // back to top
  }
  return enemy::animate(index);
}

void eBoulder::drawshadow(BITMAP *dest, bool translucent)
{
  if(clk2>=0)
  {
	  int tempy=yofs;
	  flip = 0;
	  int f2=((clk<<2)/frate)<<1;
	  shadowtile = wpnsbuf[iwLargeShadow].tile+f2;
	  yofs+=zc_max(0,zc_min(29-clk3,clk3));

	  yofs+=8;  xofs-=8;
	  enemy::drawshadow(dest, translucent);
	  xofs+=16; ++shadowtile;
	  enemy::drawshadow(dest, translucent);
	  yofs+=16;           shadowtile+=20;
	  enemy::drawshadow(dest, translucent);
	  xofs-=16; --shadowtile;
	  enemy::drawshadow(dest, translucent);
	  xofs+=8;
	  yofs=tempy;
  }
}

void eBoulder::draw(BITMAP *dest)
{
  if(clk2>=0)
  {
    int tempdir=dir;
    dir=dummy_int[2];
    update_enemy_frame();
    dir=tempdir;
    xofs-=8; yofs-=8;
    drawblock(dest,15);
    xofs+=8; yofs+=8;
    //    enemy::draw(dest);
  }
}

int eBoulder::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnId=wpnId;
  power=power;
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  return 0;
}

eProjectile::eProjectile(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  /* fixing
    hp=1;
    */
  mainguy=false;
  count_enemy=false;
  superman=1;
  hxofs=1000;
  hclk=clk;                                                 // the "no fire" range
  clk=96;
}

bool eProjectile::animate(int)
{
  double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
  if ((ddir<=(((-1)*PI)/4))&&(ddir>(((-3)*PI)/4)))
  {
    dir=down;
  }
  else if ((ddir<=(((1)*PI)/4))&&(ddir>(((-1)*PI)/4)))
  {
    dir=right;
  }
  else if ((ddir<=(((3)*PI)/4))&&(ddir>(((1)*PI)/4)))
  {
    dir=up;
  }
  else
  {
    dir=left;
  }
  if(++clk>80)
  {
    unsigned r=rand();
    if(!(r&63) && !LinkInRange(hclk))
    {
//      addEwpn(x,y,z,d->weapon,0,d->wdp,dir);
      addEwpn(x,y,z,wpn,0,wdp,dir,getUID());
//      if (d->weapon==ewFireball)
      if (wpn==ewFireball || wpn==ewFireball2)
      {
        if(!((r>>7)&15))
        {
//          addEwpn(x-4,y,z,d->weapon,0,d->wdp,dir);
          addEwpn(x-4,y,z,wpn,0,wdp,dir,getUID());
        }
      }
      clk=0;
    }
  }
  return false;
}

void eProjectile::draw(BITMAP *dest)
{
  //these are here to bypass compiler warnings about unused arguments
  dest=dest;
}

eTrigger::eTrigger(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  x=-1000; y=-1000;
}

void eTrigger::draw(BITMAP *)
{
}

void eTrigger::death_sfx()
{
	//silent death
}

eNPC::eNPC(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  o_tile+=wpnsbuf[iwNPCs].tile;
  count_enemy=false;
}

bool eNPC::animate(int index)
{
  if(dying)
    return Dead();

  switch (dmisc2)
  {
    //case eNPCSTAND1:
    //case eNPCSTAND2:
    //case eNPCSTAND3:
    //case eNPCSTAND4:
    //case eNPCSTAND5:
    //case eNPCSTAND6:
  case 0:
    {
      double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
      if ((ddir<=(((-1)*PI)/4))&&(ddir>(((-3)*PI)/4)))
      {
        dir=down;
      }

      else if ((ddir<=(((1)*PI)/4))&&(ddir>(((-1)*PI)/4)))
        {
          dir=right;
        }
        else if ((ddir<=(((3)*PI)/4))&&(ddir>(((1)*PI)/8)))
          {
            dir=up;
          }
          else
          {
            dir=left;
          }
    }
    break;
    //case eNPCWALK1:
    //case eNPCWALK2:
    //case eNPCWALK3:
    //case eNPCWALK4:
    //case eNPCWALK5:
    //case eNPCWALK6:
  case 1:
//    halting_walk(d->rate, d->homing, 0, d->hrate, 48);
    halting_walk(rate, homing, 0, hrate, 48);

//    if(clk2==1 && (misc < d->misc1) && !(rand()&15))
    if(clk2==1 && (misc < dmisc1) && !(rand()&15))
    {
//      newdir(d->rate, d->homing, 0);
      newdir(rate, homing, 0);
      clk2=48;
      ++misc;
    }

    if(clk2==0)
      misc=0;

    break;
  }

  return enemy::animate(index);
}

void eNPC::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

int eNPC::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnId=wpnId;
  power=power;
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  return 0;
}

eSpinTile::eSpinTile(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  o_tile=clk;
  cs=id>>12;
  id=id&0xFFF;
  clk=0;
  step=0;
  count_enemy=false;
}

bool eSpinTile::animate(int index)
{
  if(dying)
  {
    return Dead();
  }
  ++misc;
  if (misc==96)
  {
    double ddir=atan2(double((Link.y)-y),double(Link.x-x));
    angular=true;
    angle=ddir;
    step=3;
  }
  sprite::move(step);
  return enemy::animate(index);
}

void eSpinTile::draw(BITMAP *dest)
{
  update_enemy_frame();
  if (misc>=96)
  {
    tile+=4;
  }
  y-=(misc>>4);
  yofs+=2;
  enemy::draw(dest);
  yofs-=2;
  y+=(misc>>4);
}

void eSpinTile::drawshadow(BITMAP *dest, bool translucent)
{
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile+(clk%4);
  yofs+=4;
  enemy::drawshadow(dest, translucent);
  yofs-=4;
}

int eSpinTile::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnId=wpnId;
  power=power;
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  return 0;
}

eZora::eZora(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,0)
{
  //these are here to bypass compiler warnings about unused arguments
  Clk=Clk;

  mainguy=false;
  count_enemy=false;
  /*if((x>-17 && x<0) && iswater(tmpscr->data[(((int)y&0xF0)+((int)x>>4))]))
  {
    clk=1;
  }*/
  //nets+880;
}

void eZora::facelink()
{
  if (Link.x-x==0)
  {
    dir=(Link.y+8<y)?up:down;
  }
  else
  {
    double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
    if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
    {
      dir=l_down;
    }
    else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
    {
      dir=down;
    }
    else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
    {
      dir=r_down;
    }
    else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
    {
      dir=right;
    }
    else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
    {
      dir=r_up;
    }
    else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
    {
      dir=up;
    }
    else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
    {
      dir=l_up;
    }
    else
    {
      dir=left;
    }
  }
}

bool eZora::animate(int index)
{
  if(dying)
    return Dead();

  if(watch)
  {
    ++clock_zoras[id];
    return true;
  }

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    facelink();
  }
  switch(clk)
  {
    case 0:                                                 // reposition him
    {
      int t=0;
      int pos2=rand()%160 + 16;
      bool placed=false;
      while(!placed && t<160)
      {
        if(iswater(tmpscr->data[pos2]) && (pos2&15)>0 && (pos2&15)<15)
        {
          x=(pos2&15)<<4;
          y=pos2&0xF0;
          hp=guysbuf[id].hp;                             // refill life each time
          hxofs=1000;                                       // avoid hit detection
          stunclk=0;
          placed=true;
        }
        pos2+=19;
        if(pos2>=176)
          pos2-=160;
        ++t;
      }
      if(!placed || whistleclk>=88)                         // can't place him, he's gone
        return true;

    } break;
    case 35:
    if (!get_bit(quest_rules,qr_NEWENEMYTILES))
    {
      dir=(Link.y+8<y)?up:down;
    }
    hxofs=0; break;
//    case 35+19: addEwpn(x,y,z,ewFireball,0,d->wdp,0); break;
    case 35+19: addEwpn(x,y,z,wpn,0,wdp,dir,getUID()); break;
    case 35+66: hxofs=1000; break;
    case 198:   clk=-1; break;
  }

  return enemy::animate(index);
}

void eZora::draw(BITMAP *dest)
{
  if(clk<3)
    return;

  update_enemy_frame();
  enemy::draw(dest);
}

eStalfos::eStalfos(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  doubleshot=false;

  //nets+2380;
}

bool eStalfos::animate(int index)
{
  if(dying)
    return Dead();
  if(family==eeWALK/*id==eSTALFOS*/)
    //constant_walk(4,128,0);
    constant_walk(rate,homing,0);
  else
  {
    //halting_walk(4,128,0,4,48);
	halting_walk(rate,homing,0,hrate,48);
    if(clk2==16 && sclk==0 && !stunclk && !watch)
    {
      if (dmisc1)
      {
//        Ewpns.add(new weapon(x,y,z,ewSword,0,d->wdp,up));
//        Ewpns.add(new weapon(x,y,z,ewSword,0,d->wdp,down));
//        Ewpns.add(new weapon(x,y,z,ewSword,0,d->wdp,left));
//        Ewpns.add(new weapon(x,y,z,ewSword,0,d->wdp,right));
        Ewpns.add(new weapon(x,y,z,wpn,0,wdp,up,getUID()));
        Ewpns.add(new weapon(x,y,z,wpn,0,wdp,down,getUID()));
        Ewpns.add(new weapon(x,y,z,wpn,0,wdp,left,getUID()));
        Ewpns.add(new weapon(x,y,z,wpn,0,wdp,right,getUID()));
      }
      else
      {
//        Ewpns.add(new weapon(x,y,z,ewSword,0,d->wdp,dir));
        Ewpns.add(new weapon(x,y,z,wpn,0,wdp,dir,getUID()));
      }
    }
    if(clk2==1 && !doubleshot && !(rand()&15))
    {
      newdir(4,128,0);
      clk2=48;
      doubleshot=true;
    }
    if(clk2==0)
      doubleshot=false;
  }

  return enemy::animate(index);
}

void eStalfos::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eGibdo::eGibdo(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //nets+560;
}

bool eGibdo::animate(int index)
{
  if(dying)
    return Dead();
  constant_walk(rate,homing,0);

  return enemy::animate(index);
}

void eGibdo::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eBubble::eBubble(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  superman=1;
  mainguy=false;
  //nets+300;
}

bool eBubble::animate(int index)
{
  if(dying)
    return Dead();
  constant_walk(rate,homing,0);
  return enemy::animate(index);
}

void eBubble::draw(BITMAP *dest)
{
  update_enemy_frame();
  if(((flags2&guy_flashing)/*||(id==eBUBBLEIT)*/)&&(get_bit(quest_rules,qr_NOBUBBLEFLASH)))
  {
//    cs = d->cset;
    cs = dcset;
  }
  enemy::draw(dest);
}

eRope::eRope(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  didbomb=false;
  charging=false;
  //nets+840;
}

bool eRope::animate(int index)
{
  if(dmisc1/*id==eBOMBCHU*/)
  {
    if (!didbomb&&charging&&LinkInRange(16))
    {
      hp=-1000;
//      weapon *ew=new weapon(x,y,z, ewSBomb, 0, d->wdp*4, dir);
      if(wpn+dmisc2 > wEnemyWeapons && wpn+dmisc2 < wMax)
      {
          weapon *ew=new weapon(x,y,z, wpn+dmisc2, 0, dmisc3, dir,getUID());
          Ewpns.add(ew);
          if (wpn==ewSBomb || wpn==ewBomb)
          {
            ew->step=0;
            ew->id=wpn+dmisc2;
		  ew->misc=50;
            ew->clk=48;
          }
          didbomb=true;
      }
      else
      {
        weapon *ew=new weapon(x,y,z, wpn, 0, dmisc3, dir,getUID());
          Ewpns.add(ew);
          if (wpn==ewSBomb || wpn==ewBomb)
          {
            ew->step=0;
            ew->id=wpn;
		  ew->misc=50;
            ew->clk=48;
          }
          didbomb=true;
      }
    }
    else
    {
      if(!didbomb&&hp<=0&&hp>-1000)
      {
        hp=-1000;
//        weapon *ew=new weapon(x,y,z, ewBomb, 0, d->wdp, dir);
        weapon *ew=new weapon(x,y,z, wpn, 0, wdp, dir,getUID());
        Ewpns.add(ew);
        ew->step=0;
        ew->id=wpn;
	    ew->misc=50;
        ew->clk=48;
        didbomb=true;
      }
    }
  }
  if(dying)
    return Dead();
  charge_attack();

  return enemy::animate(index);
}

void eRope::charge_attack()
{
  if(slide())
    return;
  if(clk<0 || dir<0 || stunclk || watch)
    return;

  if(clk3<=0)
  {
    x=(fix)(int(x)&0xF0);
    y=(fix)(int(y)&0xF0);
    if(!charging)
    {
      int ldir = lined_up(7,false);
      if(ldir!=-1 && canmove(ldir))
      {
        dir=ldir;
        charging=true;
        step=(dstep/100.0)+1;
      }
      else newdir(4,0,0);
    }
    if(!canmove(dir))
    {
      newdir();
      charging=false;
      step=dstep/100.0;
    }
    clk3=int(16.0/step);
  }

  move(step);
  --clk3;
}

void eRope::draw(BITMAP *dest)
{
  if((dmisc1)&&hp==-1000)
  {
    return;
  }
  update_enemy_frame();
  if((flags2&guy_flashing)&&(get_bit(quest_rules,qr_NOROPE2FLASH)))
  {
//    cs = d->cset;
    cs = dcset;
  }
  if((dmisc1)&&charging)
  {
    tile+=20;
  }
  enemy::draw(dest);
}

eKeese::eKeese(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dir=(rand()&7)+8;
  step=0;
  movestatus=1;
  c=0;
  hxofs=2;
  hxsz=12;
  hyofs=4;
  hysz=8;

  //nets;
  dummy_int[1]=0;
}

bool eKeese::animate(int index)
{
  if(dying)
    return Dead();
  if (dmisc1)
  {
    floater_walk(2,8,(fix)1,(fix)0,10,0,0);
  }
  else
  {
    floater_walk(2,8,(fix)0.625,(fix)0.0625,10,120,16);
  }
  if (get_bit(quest_rules,qr_ENEMIESZAXIS) && !(tmpscr->flags7&fSIDEVIEW))
  {
    z=int(step/(fix)0.625);
    // Some variance in keese flight heights when away from Link
    z+=int(step*zc_max(0,(distance(x,y,LinkX(),LinkY())-128)/10));
  }

  return enemy::animate(index);
}

void eKeese::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile+posframe;
  yofs+=8;
  if(!get_bit(quest_rules,qr_ENEMIESZAXIS))
    yofs+=int(step/(fix)0.0625);
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eKeese::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eVire::eVire(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dir = rand()&3;
  //nets+180;
}

bool eVire::animate(int index)
{
	//if(dying);
 //  if(hp>0 && hp<guysbuf[eVIRE].hp && !fslide())
  if(hp>0 && hp<guysbuf[id].hp && !fslide())
  {
    int kids = guys.Count();
    int id2=dmisc2;
    for(int i=0;i<dmisc3;i++)
    {
      if(addenemy(x,y,id2,-24))
		((enemy*)guys.spr(kids+i))->count_enemy = false;
    }
    return true;
  }

  vire_hop();
  return enemy::animate(index);
}

void eVire::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  shadowtile = wpnsbuf[iwShadow].tile+f2;
  yofs+=8;
  if (dir>=left && !get_bit(quest_rules,qr_ENEMIESZAXIS))
  {
    yofs+=(((int)y+17)&0xF0)-y;
    yofs-=3;
  }
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eVire::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

void eVire::vire_hop()
{
  if(slide())
  {
    return;
  }
  if(clk<0 || dying || stunclk || watch)
  {
    return;
  }
  y=floor_y;
  int jump_width=1;
  if(clk3<=0) //if we've completed the move to another tile
  {
    fix_coords();
    z=0;
    //if we're not in the middle of a jump or if we can't complete the current jump in the current direction
    if(clk2<=0 || !canmove(dir,(fix)1,spw_none) || (tmpscr->flags7&fSIDEVIEW && _walkflag(x,y+16,0)))
    {
      newdir(4,64,spw_none);
    }
    if(dir>=left) //if we're moving left or right
    {
//      floor_y=y;
//    if(!canmove(dir,(fix)2,spw_none) || m_walkflag(x,y,spw_none) || !(rand()%3))
      {
        clk2=16*jump_width/step;
      }
    }
    clk3=16/step;
  }
  --clk3;
    move(step);
    floor_y=y;
    --clk2;
    //if we're in the middle of a jump
    int jump_height=16;
    if(clk2>0)
    {
      int h = fixtoi(fixsin(itofix(clk2*128*step/(16*jump_width)))*jump_height);  //fix h = (fix)((31-clk3)*0.125 - 2.0);
      if(get_bit(quest_rules,qr_ENEMIESZAXIS) && !(tmpscr->flags7&fSIDEVIEW))
      {
        z=h;
      }
      else
      {
        y=floor_y-h;
//      shadowdistance+=h;
      }
    }
}

eKeeseTrib::eKeeseTrib(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  clk4=0;
  dir=(rand()&7)+8;
  movestatus=1;
  c=0;
  hxofs=2;
  hxsz=12;
  hyofs=4;
  hysz=8;
  //nets+120;
  dummy_int[1]=0;
}

bool eKeeseTrib::animate(int index)
{
  ++clk4;
  if(dying)
    return Dead();
  floater_walk(2,8,(fix)0.625,(fix)0.0625,10,120,16);
  if (clk4==256)
  {
    if (!m_walkflag(x,y,0))
    {
      int kids = guys.Count();
      //        guys.add(new eVire(x,y,eVIRE,-24));
      //        keesetribgrow(x, y);
	  bool success = false;
	  if(get_bit(quest_rules, qr_OLDTRIBBLES))
	  {
		int id2=dmisc2;//(id==eFGELTRIB?eFZOL:eZOL);
		success = addenemy((fix)x,(fix)y,id2,-24);
	  }
	  else
	  {
		int id2=dmisc3;//(id==eFGELTRIB?eFZOLTRIB:eZOLTRIB);
		success = addenemy((fix)x,(fix)y,id2,-24);
	  }
	  if(success)
		((enemy*)guys.spr(kids))->count_enemy = count_enemy;
      return true;
    }
    else
    {
      clk4=0;
    }
  }

  return enemy::animate(index);
}

void eKeeseTrib::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  shadowtile = wpnsbuf[iwShadow].tile+posframe;
  yofs+=8;
  yofs+=int(step/(fix)0.0625);
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eKeeseTrib::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eVireTrib::eVireTrib(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  dir = rand()&3;
  //nets+160;
}

bool eVireTrib::animate(int index)
{
  if(dying)
    return Dead();
//  if(hp>0 && hp<guysbuf[eKEESETRIB+1].hp && !fslide())
  if(hp>0 && hp<guysbuf[id].hp && !fslide())
  {
    int kids = guys.Count();
	int id2=dmisc2;//(id==eFZOL?eFGEL:eGEL);
	for(int i=0;i<dmisc3;i++)
	{
	  if(addenemy(x,y,id2,-24))
		((enemy*)guys.spr(kids+i))->count_enemy = false;
	}
    return true;
  }
  vire_hop();
  return enemy::animate(index);
}

void eVireTrib::drawshadow(BITMAP *dest, bool translucent)
{
  int tempy=yofs;
  flip = 0;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  shadowtile = wpnsbuf[iwShadow].tile+f2;
  yofs+=8;
  if (dir>=left && !get_bit(quest_rules,qr_ENEMIESZAXIS))
  {
    yofs+=(((int)y+17)&0xF0)-y;
  }
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void eVireTrib::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

void eVireTrib::vire_hop()
{
  if(slide())
  {
    return;
  }
  if(clk<0 || dying || stunclk || watch)
  {
    return;
  }
  y=floor_y;
  int jump_width=1;
  if(clk3<=0) //if we've completed the move to another tile
  {
    fix_coords();
    z=0;
    //if we're not in the middle of a jump or if we can't complete the current jump in the current direction
    if(clk2<=0 || !canmove(dir,(fix)1,spw_none) || (tmpscr->flags7&fSIDEVIEW && _walkflag(x,y+16,0)))
    {
      newdir(4,64,spw_none);
    }
    if(dir>=left) //if we're moving left or right
    {
//      floor_y=y;
//    if(!canmove(dir,(fix)2,spw_none) || m_walkflag(x,y,spw_none) || !(rand()%3))
      {
        clk2=16*jump_width/step;
      }
    }
    clk3=16/step;
  }
  --clk3;
    move(step);
    floor_y=y;
    --clk2;
    //if we're in the middle of a jump
    int jump_height=16;
    if(clk2>0)
    {
      int h = fixtoi(fixsin(itofix(clk2*128*step/(16*jump_width)))*jump_height);  //fix h = (fix)((31-clk3)*0.125 - 2.0);
      if(get_bit(quest_rules,qr_ENEMIESZAXIS) && !(tmpscr->flags7&fSIDEVIEW))
      {
        z=h;
      }
      else
      {
        y=floor_y-h;
//      shadowdistance+=h;
      }
    }

}

ePolsVoice::ePolsVoice(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  shadowdistance=0;
  //nets+580;
}

bool ePolsVoice::animate(int index)
{
  if(dying)
    return Dead();
  polsvoice_hop();
  return enemy::animate(index);

}

void ePolsVoice::drawshadow(BITMAP *dest, bool translucent)
{
  /*if (get_bit(quest_rules,qr_ENEMIESZAXIS)) {

    return;
  }*/

  int tempy=yofs;
  flip = 0;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  shadowtile = wpnsbuf[iwShadow].tile;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    shadowtile+=f2;
  }
  else
  {
    shadowtile+=f2?1:0;
  }
  yofs+=8;
  yofs-=shadowdistance;
  enemy::drawshadow(dest, translucent);
  yofs=tempy;
}

void ePolsVoice::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

int ePolsVoice::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(dying || clk<0 || hclk>0 || superman)
    return 0;

  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wBomb:
    case wSBomb:
    case wLitBomb:
    case wLitSBomb:
    case wFire:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    return 0;
    case wBrang:
    if (!current_item_power(itype_brang))
    {
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    }
    else
    {
      stunclk=160;
      break;
    }
    case wHookshot:
    stunclk=160;
    break;
    case wMagic:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
    case wArrow:
    hp=0;
    break;
    default:
    hp-=power;
  }
  hclk=33;
  sfx(WAV_EHIT,pan(int(x)));
  return wpnId==wArrow ? 0 : 1;                             // arrow keeps going
}

void ePolsVoice::polsvoice_hop()
{
  if(slide())
  {
    return;
  }
  if(clk<0 || dying || stunclk || watch)
  {
    return;
  }
  y=floor_y;
  int jump_width=2;
  if(clk3<=0) //if we've completed the move to another tile
  {
    fix_coords();
//  z=0;
    //if we're not in the middle of a jump or if we can't complete the current jump in the current direction
    if(clk2<=0 || !canmove(dir,(fix)1,spw_floater) || (tmpscr->flags7&fSIDEVIEW && _walkflag(x,y+16,0)))
    {
      newdir(4,32,spw_floater);
    }
    if(clk2<=0) //if we're not in the middle of a jump
    {
//      floor_y=y;
      if(!canmove(dir,(fix)2,spw_none) || m_walkflag(x,y,spw_none) || !(rand()%3))
      {
        clk2=16*jump_width/step;
      }
    }
    clk3=16/step;
  }
  --clk3;
    move(step);
    floor_y=y;
    --clk2;
    //if we're in the middle of a jump
    int jump_height=27;
    if(clk2>0)
    {
      int h = fixtoi(fixsin(itofix(clk2*128*step/(16*jump_width)))*jump_height);
      if(get_bit(quest_rules,qr_ENEMIESZAXIS) && !(tmpscr->flags7&fSIDEVIEW))
      {
        z=h;
      }
      else
      {
        y=floor_y-h;
//      shadowdistance+=h;
      }
    }

}

eLikeLike::eLikeLike(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //nets+280;
  haslink=false;
}

bool eLikeLike::animate(int index)
{
  if(hp<=0 && haslink)
  {
    Link.beatlikelike();
    haslink=false;
  }
  if(dying)
    return Dead();

  if(haslink)
  {
    ++clk2;
    if(clk2==95)
	{
		for(int i=0;i<MAXITEMS;i++)
		{
		  if (itemsbuf[i].flags&ITEM_EDIBLE)
		    game->set_item(i, false);
		}
	}
    if((clk&0x18)==8)                                       // stop its animation on the middle frame
      --clk;
  }
  else
    constant_walk(rate,homing,spw_none);

  return enemy::animate(index);
}

void eLikeLike::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

void eLikeLike::eatlink()
{
  haslink=true;
  if (Link.isSwimming())
  {
    Link.setX(x);
    Link.setY(y);
  }
  else
  {
    x=Link.getX();
    y=Link.getY();
  }
  clk2=0;
}

eShooter::eShooter(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
}

bool eShooter::animate(int index)
{
  if(dying)
    return Dead();

//  halting_walk(d->rate, d->homing, 0, d->hrate, 48);
  halting_walk(rate, homing, 0, hrate, 48);

  if(clk2==16 && sclk==0 && !stunclk && !watch)
  {
//    Ewpns.add(new weapon(x,y,z, d->weapon, 0, d->wdp, dir));
    Ewpns.add(new weapon(x,y,z, wpn, 0, wdp, dir,getUID()));
  }

//  if(clk2==1 && (misc < d->misc1) && !(rand()&15))
  if(clk2==1 && (misc < dmisc1) && !(rand()&15))
  {
//    newdir(d->rate, d->homing, 0);
    newdir(rate, homing, 0);
    clk2=48;
    ++misc;
  }

  if(clk2==0)
    misc=0;

  return enemy::animate(index);
}

void eShooter::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eOctorok::eOctorok(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //nets+1840;
  timer=0;
  dummy_bool[0]=false;
}

bool eOctorok::animate(int index)
{
  if(dying)
  {
    if (dmisc1==1||dmisc1==2)
    {
      //Kamikaze Fire Octo!
      if (!dummy_bool[0])
      {
	    int wpn2 = wpn+dmisc2;
		if(wpn2 <= wEnemyWeapons || wpn2 >= wMax)
			wpn2=wpn;
        dummy_bool[0]=true;
//        addEwpn(x,y,z,ewFlame,0,d->wdp*4,up);
//        addEwpn(x,y,z,ewFlame,0,d->wdp*4,down);
//        addEwpn(x,y,z,ewFlame,0,d->wdp*4,left);
//        addEwpn(x,y,z,ewFlame,0,d->wdp*4,right);
//        addEwpn(x,y,z,ewFlame,0,d->wdp*4,l_up);
//        addEwpn(x,y,z,ewFlame,0,d->wdp*4,r_up);
//        addEwpn(x,y,z,ewFlame,0,d->wdp*4,l_down);
//        addEwpn(x,y,z,ewFlame,0,d->wdp*4,r_down);
        addEwpn(x,y,z,wpn2,0,dmisc3,up,getUID());
        addEwpn(x,y,z,wpn2,0,dmisc3,down,getUID());
        addEwpn(x,y,z,wpn2,0,dmisc3,left,getUID());
        addEwpn(x,y,z,wpn2,0,dmisc3,right,getUID());
        addEwpn(x,y,z,wpn2,0,dmisc3,l_up,getUID());
        addEwpn(x,y,z,wpn2,0,dmisc3,r_up,getUID());
        addEwpn(x,y,z,wpn2,0,dmisc3,l_down,getUID());
        addEwpn(x,y,z,wpn2,0,dmisc3,r_down,getUID());

        sfx(WAV_FIRE,pan(int(x)));
      }
    }
    return Dead();
  }

//  halting_walk(d->rate, d->homing, 0, d->hrate, 48);
  halting_walk(rate, homing, 0, hrate, 48);

  if(clk2==16 && sclk==0 && !stunclk && !watch)
  {
    if (dmisc1==1||dmisc1==2)
    {
      timer=rand()%50+50;
    }
    else
    {
//      weapon *ew=new weapon(x,y,z,d->weapon, 0, d->wdp, dir);
      weapon *ew=new weapon(x,y,z, wpn, 0, wdp, dir,getUID());
      if (wpn == ewMagic && get_bit(quest_rules,qr_MORESOUNDS))
        sfx(WAV_WAND,pan(int(x)));
      Ewpns.add(ew);
      if (dmisc1==3)
      {
        ew->step=3;
      }
    }
  }

//  if(clk2==1 && (misc < d->misc1) && !(rand()&15))
  if(clk2==1 && (misc < dmisc4) && !(rand()&15))
  {
//    newdir(d->rate, d->homing, 0);
    newdir(rate, homing, 0);
    clk2=48;
    ++misc;
  }

  if (timer) //Fire Octo
  {
    if (dmisc1==1)
    {
      clk2=15; //this keeps the octo in place until he's done firing
    }
    if (!(timer%4))
    {
//      addEwpn(x,y+4,z,ewFlame,0,4*DAMAGE_MULTIPLIER,255);
      float fire_angle=0.0;
      int wx=0, wy=0;
      switch (dir)
      {
        case down:
          fire_angle=PI*((rand()%20)+10)/40;
          wx=x;
          wy=y+8;
          break;
        case up:
          fire_angle=PI*((rand()%20)+50)/40;
          wx=x;
          wy=y-8;
          break;
        case left:
          fire_angle=PI*((rand()%20)+30)/40;
          wx=x-8;
          wy=y;
          break;
        case right:
          fire_angle=PI*((rand()%20)+70)/40;
          wx=x+8;
          wy=y;
          break;
      }
//      addEwpn(wx,wy,z,d->weapon,0,d->wdp,0);
      addEwpn(wx,wy,z,wpn,0,wdp,0,getUID());
      sfx(WAV_FIRE,pan(int(x)));

      int i=Ewpns.Count()-1;
      weapon *ew = (weapon*)(Ewpns.spr(i));
      ew->angular=true;
      ew->angle=fire_angle;
//      if (wpnsbuf[ewFLAME].frames>1)
      if (wpnsbuf[wpn].frames>1)
      {
        ew->aframe=rand()%wpnsbuf[wpn].frames;
        ew->tile+=ew->aframe;
      }
      for(int j=Ewpns.Count()-1; j>0; j--)
      {
        Ewpns.swap(j,j-1);
      }
    }
    --timer;
  }

  if(clk2==0)
    misc=0;

  return enemy::animate(index);
}

void eOctorok::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eMoblin::eMoblin(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //nets+2260;
}

bool eMoblin::animate(int index)
{
  if(dying)
    return Dead();

//  halting_walk(d->rate, d->homing, 0, d->hrate, 48);
  halting_walk(rate, homing, 0, hrate, 48);

  if(clk2==16 && sclk==0 && !stunclk && !watch)
  {
//    Ewpns.add(new weapon(x,y,z, d->weapon, 0, d->wdp, dir));
    Ewpns.add(new weapon(x,y,z, wpn, 0, wdp, dir,getUID()));
  }

//  if(clk2==1 && (misc < d->misc1) && !(rand()&15))
  if(clk2==1 && (misc < dmisc1) && !(rand()&15))
  {
//    newdir(d->rate, d->homing, 0);
    newdir(rate, homing, 0);
    clk2=48;
    ++misc;
  }

  if(clk2==0)
    misc=0;

  return enemy::animate(index);
}

void eMoblin::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eLynel::eLynel(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //nets+2520;
}

bool eLynel::animate(int index)
{
  if(dying)
    return Dead();

//  halting_walk(d->rate, d->homing, 0, d->hrate, 48);
  halting_walk(rate, homing, 0, hrate, 48);

  if(clk2==16 && sclk==0 && !stunclk && !watch)
  {
//    Ewpns.add(new weapon(x,y,z, d->weapon, 0, d->wdp, dir));
    Ewpns.add(new weapon(x,y,z, wpn, 0, wdp, dir,getUID()));
  }

//  if(clk2==1 && (misc < d->misc1) && !(rand()&15))
  if(clk2==1 && (misc < dmisc1) && !(rand()&15))
  {
//    newdir(d->rate, d->homing, 0);
    newdir(rate, homing, 0);
    clk2=48;
    ++misc;
  }

  if(clk2==0)
    misc=0;

  return enemy::animate(index);
}

void eLynel::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eGoriya::eGoriya(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //nets+2140;
}

bool eGoriya::animate(int index)
{
  if(dying)
  {
    KillWeapon();
    return Dead();
  }
  if(!WeaponOut())
  {
    halting_walk(rate,homing,0,hrate,1);
    if(clk2==1 && sclk==0 && !stunclk && !watch && wpn)
    {
//      Ewpns.add(new weapon(x,y,z,ewBrang,misc,d->wdp,dir));
      Ewpns.add(new weapon(x,y,z,wpn,misc,wdp,dir,getUID()));
      ((weapon*)Ewpns.spr(Ewpns.Count()-1))->dummy_bool[0]=false;
      if (dmisc1==2)
      {
        int ndir=dir;
        if (Link.x-x==0)
        {
          ndir=(Link.y+8<y)?up:down;
        }
        else //turn to face Link
        {
          double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
          if ((ddir<=(((-2)*PI)/8))&&(ddir>(((-6)*PI)/8)))
          {
            ndir=down;
          }
          else if ((ddir<=(((2)*PI)/8))&&(ddir>(((-2)*PI)/8)))
          {
            ndir=right;
          }
          else if ((ddir<=(((6)*PI)/8))&&(ddir>(((2)*PI)/8)))
          {
            ndir=up;
          }
          else
          {
            ndir=left;
          }
        }
        ((weapon*)Ewpns.spr(Ewpns.Count()-1))->dummy_bool[0]=true;
        if (canmove(ndir))
        {
          dir=ndir;
        }
      }
    }
  }
  else if(clk2>2)
  {
    --clk2;
  }

  return enemy::animate(index);
}

bool eGoriya::WeaponOut()
{
  for(int i=0; i<Ewpns.Count(); i++)
  {
    if(((weapon*)Ewpns.spr(i))->parentid==getUID() && Ewpns.spr(i)->id==ewBrang)
    {
      return true;
    }
  }
  return false;
}

void eGoriya::KillWeapon()
{
  for(int i=0; i<Ewpns.Count(); i++)
  {
    if(((weapon*)Ewpns.spr(i))->type==misc && Ewpns.spr(i)->id==ewBrang)
    {
      Ewpns.del(i);
    }
  }
  if (!Ewpns.idCount(ewBrang))
  {
    stop_sfx(WAV_BRANG);
  }
}

void eGoriya::draw(BITMAP *dest)
{
  update_enemy_frame();
  enemy::draw(dest);
}

eDarknut::eDarknut(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  noshield=false;
  fired=false;
  //nets+2640;
}

bool eDarknut::animate(int index)
{
  if(dying)
    return Dead();
  constant_walk(rate,homing,grumble);
  switch (dmisc1)
  {
    //case eDKNUT1:
    //constant_walk(rate,homing,grumble);
    //break;
    //case eDKNUT2:
    //constant_walk(4,160,0);
    //break;
    //case eDKNUT3:
    //constant_walk(3,200,0);
  case 2:
    if(hp<=0)
    {
      int kids = guys.Count();
	  int id2=dmisc2;//(id==eFZOL?eFGEL:eGEL);
	  for(int i=0;i<dmisc3;i++)
	  {
	    if(addenemy(x,y,id2+((i+1)<<12),-22))
			((enemy*)guys.spr(kids+i))->count_enemy = false;
	  }
    }
    break;
    //case eDKNUT5:
    //switch later to:  halting_walk(6,128,0,(id==eGORIYA1)?5:6,1);
    //constant_walk(2,255,0);
  case 1:
    if(sclk==0 && !stunclk && !watch)
    {
      if (clk2>64)
      {
        clk2=0;
        fired=false;
      }
      clk2+=rand()&3;
      if ((clk2>24)&&(clk2<52))
      {
        tile+=20;                                         //firing
        if (!fired&&(clk2>=38))
        {
//          Ewpns.add(new weapon(x,y,z, d->weapon, 0, d->wdp, dir));
          Ewpns.add(new weapon(x,y,z, wpn, 0, wdp, dir,getUID()));
          fired=true;
        }
      }
    }
    break;
  }

  return enemy::animate(index);
}

void eDarknut::draw(BITMAP *dest)
{
  bool tempbool=dummy_bool[1];
  dummy_bool[1]=noshield;
  update_enemy_frame();
  dummy_bool[1]=tempbool;
  enemy::draw(dest);
}


int eDarknut::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(dying || clk<0 || hclk>0 || (superman && !(superman>1 && wpnId==wSBomb)))
    return 0;

  switch(wpnId)
  {
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wFire:
    case wWind:
    case wSSparkle:
    case wFSparkle:
    case wPhantom:
    return 0;
    case wBomb:
    case wSBomb:
    case wRefMagic:
    case wRefRock:
    case wHookshot:
    case wHammer:
    case wStomp:
    goto skip1;
  }


  if((wpnDir==(dir^1)) && !noshield)
  {
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
  }

skip1:

  switch(wpnId)
  {
    case wBrang:
      if ((!current_item_power(itype_brang)) || ((dir==up)&&(wpny<y))|| ((dir==down)&&(wpny>y)) || ((dir==left)&&(wpnx<x)) || ((dir==right)&&(wpnx>x)) )
      {
        sfx(WAV_CHINK,pan(int(x)));
        return 1;
      }
      else
      {
        return enemy::takehit(wpnId,power,wpnx,wpny,wpnDir);
      }
      break;

    case wHookshot:
      if (((dir==up)&&(wpny<y))|| ((dir==down)&&(wpny>y)) || ((dir==left)&&(wpnx<x)) || ((dir==right)&&(wpnx>x)) && (!noshield) )
      {
        sfx(WAV_CHINK,pan(int(x)));
        return 1;
      }
      else
      {
        return enemy::takehit(wpnId,power,wpnx,wpny,wpnDir);
      }
      break;

    case wArrow:
    case wMagic:
    case wStomp:
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    case wBomb:
    case wSBomb:
      if(get_bit(quest_rules,qr_BOMBDARKNUTFIX))
      {
        double ddir=atan2(double(wpny-y),double(x-wpnx));
        wpnDir=rand()&3;
        if ((ddir<=(((-1)*PI)/4))&&(ddir>(((-3)*PI)/4)))
        {
          wpnDir=down;
        }
        else if ((ddir<=(((1)*PI)/4))&&(ddir>(((-1)*PI)/4)))
        {
          wpnDir=right;
        }
        else if ((ddir<=(((3)*PI)/4))&&(ddir>(((1)*PI)/4)))
        {
          wpnDir=up;
        }
        else
        {
          wpnDir=left;
        }
      }
      if((wpnDir==(dir^1)) && !noshield)
      {
        break;
      }
      //partial fall through
    case wHammer:
      if (wpnDir==(dir^1)&&get_bit(quest_rules,qr_BRKBLSHLDS))
      {
        noshield=true;
      }
      //fall through
    default:
      hp-=power;
      break;
  }

  hclk=33;
  if((dir&2)==(wpnDir&2))
    sclk=(wpnDir<<8)+16;
  sfx(WAV_EHIT,pan(int(x)));
  return 1;
}

eWizzrobe::eWizzrobe(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
//  switch(d->misc1)
  switch(dmisc1)
  {
    case 0:
    hxofs=1000;
    fading=fade_invisible;
    clk+=220+14;
    break;
    default: dir=(loadside==right)?right:left; misc=-3; break;
  }
  //netst+2880;
  charging=false;
  firing=false;
  fclk=0;
}

bool eWizzrobe::animate(int index)
{
  if(dying)
  {
    return Dead();
  }

//  if(d->misc1)                                              //walking wizzrobe
  if(dmisc1)                                              //walking wizzrobe
  {
    wizzrobe_attack();
  }
  else
  {
    if(watch)
    {
      fading=0;
      hxofs=0;
    }
    else switch(clk)
    {
      case 0:
        if (!dmisc2)
        {
          place_on_axis(true);
        }
        else
        {
          int t=0;
          bool placed=false;
          while(!placed && t<160)
          {
            if (isdungeon())
            {
              x=((rand()%12)+2)*16;
              y=((rand()%7)+2)*16;
            }
            else
            {
              x=((rand()%14)+1)*16;
              y=((rand()%9)+1)*16;
            }
            if((!m_walkflag(x,y,1))&&(abs(x-Link.getX())>=32)||(abs(y-Link.getY())>=32))
            {
              //        if(iswater(tmpscr->data[pos]) && (pos&15)>0 && (pos&15)<15) {
              //               x=(pos&15)<<4;
              //               y=pos&0xF0;
              placed=true;
            }
            ++t;
          }
          if (abs(x-Link.getX())<abs(y-Link.getY()))
          {
            if (y<Link.getY())
            {
              dir=down;
            }
            else
            {
              dir=up;
            }
          }
          else
          {
            if (x<Link.getX())
            {
              dir=right;
            }
            else
            {
              dir=left;
            }
          }

          if(!placed)                                       // can't place him, he's gone
            return true;
        }
        //         place_on_axis(true);
        fading=fade_flicker;
        hxofs=0;
        break;
      case 64:
        fading=0;
        charging=true;
        break;
      case 73:
        charging=false;
        firing=40;
        break;
      case 83:
        if (!dmisc2||dmisc2==1)//id==eWIZ1)
        {
//          addEwpn(x,y,z,ewMagic,0,d->wdp,dir);
          addEwpn(x,y,z,wpn,0,wdp,dir,getUID());
          sfx(WAV_WAND,pan(int(x)));
        }
        //else if (id==eWWIZ)
        //{
//          addEwpn(x,y,z,ewWind,0,d->wdp,dir);
          //addEwpn(x,y,z,wpn,0,wdp,dir);
          //sfx(WAV_WAND,pan(int(x)));
        //}
        else if (dmisc2==2)//id==eBATROBE)
        {
          int bc=0;
          for (int gc=0; gc<guys.Count(); gc++)
          {
            if ((((enemy*)guys.spr(gc))->id) == dmisc3)
            {
              ++bc;
            }
          }
          if (bc<=40)
          {
            int kids = guys.Count();
            int bats=(rand()%3)+1;
            for (int i=0; i<bats; i++)
            {
              //guys.add(new eKeese(x,y,dmisc3,-10));
			  if(addenemy(x,y,dmisc3,-10))
				((enemy*)guys.spr(kids+i))->count_enemy = false;
            }
            sfx(WAV_FIRE,pan(int(x)));
          }
        }
        else if (dmisc2==3)//id==eSUMMONER)
        {
          if(count_layer_enemies()==0)
          {
            break;
          }

          int kids = guys.Count();
          if (kids<200)
          {
            int newguys=(rand()%3)+1;
            bool summoned=false;
            for (int i=0; i<newguys; i++)
            {
              int id2=vbound(random_layer_enemy(),eSTART,eMAXGUYS-1);
              int x2=0;
              int y2=0;
              for (int k=0; k<20; ++k)
              {
                x2=16*((rand()%12)+2);
                y2=16*((rand()%7)+2);
                if((!m_walkflag(x2,y2,0))&&(abs(x2-Link.getX())>=32)||(abs(y2-Link.getY())>=32))
                {
                  if(addenemy(x2,y2,get_bit(quest_rules,qr_ENEMIESZAXIS) ? 64 : 0,id2,-10))
					((enemy*)guys.spr(kids+i))->count_enemy = false;
                  summoned=true;
                  break;
                }
              }
            }
            if (summoned)
            {
              sfx(get_bit(quest_rules,qr_MORESOUNDS) ? WAV_ZN1SUMMON : WAV_FIRE,pan(int(x)));
            }
          }
        }
        break;
      case 119:
        firing=false;
        charging=true;
        break;
      case 128:
        fading=fade_flicker;
        charging=false;
        break;
      case 146:
        fading=fade_invisible;
        hxofs=1000;
        break;
      case 220:
        clk=-1;
        break;
    }
  }
  return enemy::animate(index);
}


void eWizzrobe::wizzrobe_attack()
{
  if(slide())
    return;
  if(clk<0 || dying || stunclk || watch)
    return;
  if(clk3<=0 || ((clk3&31)==0 && !canmove(dir,(fix)1,spw_door) && !misc))
  {
    fix_coords();
    switch(misc)
    {
      case 1:                                               //walking
      if(!m_walkflag(x,y,spw_door))
        misc=0;
      else
      {
        clk3=16;
        if(!canmove(dir,(fix)1,spw_wizzrobe))
          newdir(4,0,spw_wizzrobe);
      }
      break;

      case 2:                                               //phasing
      {
        int jx=x;
        int jy=y;
        int jdir=-1;
        switch(rand()&7)
        {
          case 0: jx-=32; jy-=32; jdir=15; break;
          case 1: jx+=32; jy-=32; jdir=9;  break;
          case 2: jx+=32; jy+=32; jdir=11; break;
          case 3: jx-=32; jy+=32; jdir=13; break;
        }
        if(jdir>0 && jx>=32 && jx<=208 && jy>=32 && jy<=128)
        {
          misc=3;
          clk3=32;
          dir=jdir;
          break;
        }
      }

      case 3:
      dir&=3;
      misc=0;

      case 0:
      newdir(4,64,spw_wizzrobe);

      default:
      if(!canmove(dir,(fix)1,spw_door))
      {
        if(canmove(dir,(fix)15,spw_wizzrobe))
        {
          misc=1;
          clk3=16;
        }
        else
        {
          newdir(4,64,spw_wizzrobe);
          misc=0;
          clk3=32;
        }
      }
      else
      {
        clk3=32;
      }
      break;
    }
    if(misc<0)
      ++misc;
  }
  --clk3;
  switch(misc)
  {
    case 1:
    case 3:  step=1.0; break;
    case 2:  step=0;   break;
    default: step=0.5; break;

  }

  move(step);
//  if(d->misc1 && misc<=0 && clk3==28)
  if(dmisc1 && misc<=0 && clk3==28)
  {
    if (!dmisc2||dmisc2==2)
    {
      if(lined_up(8,false) == dir)
      {
//        addEwpn(x,y,z,ewMagic,0,d->wdp,dir);
        addEwpn(x,y,z,wpn,0,wdp,dir,getUID());
        sfx(WAV_WAND,pan(int(x)));
        fclk=30;
      }
    }
    else
    {
      if ((rand()%500)>=400)
      {
//        addEwpn(x,y,z,ewFlame,0,d->wdp,up);
//        addEwpn(x,y,z,ewFlame,0,d->wdp,down);
//        addEwpn(x,y,z,ewFlame,0,d->wdp,left);
//        addEwpn(x,y,z,ewFlame,0,d->wdp,right);
//        addEwpn(x,y,z,ewFlame,0,d->wdp,l_up);
//        addEwpn(x,y,z,ewFlame,0,d->wdp,r_up);
//        addEwpn(x,y,z,ewFlame,0,d->wdp,l_down);
//        addEwpn(x,y,z,ewFlame,0,d->wdp,r_down);
        addEwpn(x,y,z,wpn,0,wdp,up,getUID());
        addEwpn(x,y,z,wpn,0,wdp,down,getUID());
        addEwpn(x,y,z,wpn,0,wdp,left,getUID());
        addEwpn(x,y,z,wpn,0,wdp,right,getUID());
        addEwpn(x,y,z,wpn,0,wdp,l_up,getUID());
        addEwpn(x,y,z,wpn,0,wdp,r_up,getUID());
        addEwpn(x,y,z,wpn,0,wdp,l_down,getUID());
        addEwpn(x,y,z,wpn,0,wdp,r_down,getUID());
        sfx(WAV_FIRE,pan(int(x)));
        fclk=30;
      }
    }
  }
  if(misc==0 && (rand()&127)==0)
    misc=2;
  if(misc==2 && clk3==4)
    fix_coords();
  if (!(charging||firing))                              //should never be charging or firing for these wizzrobes
  {
    if (fclk>0)
    {
      --fclk;
    }
  }

}

void eWizzrobe::draw(BITMAP *dest)
{
//  if(d->misc1 && (misc==1 || misc==3) && (clk3&1) && hp>0 && !watch && !stunclk)                          // phasing
  if(dmisc1 && (misc==1 || misc==3) && (clk3&1) && hp>0 && !watch && !stunclk)                          // phasing
    return;

  int tempint=dummy_int[1];
  bool tempbool1=dummy_bool[1];
  bool tempbool2=dummy_bool[2];
  dummy_int[1]=fclk;
  dummy_bool[1]=charging;
  dummy_bool[2]=firing;
  update_enemy_frame();
  dummy_int[1]=tempint;
  dummy_bool[1]=tempbool1;
  dummy_bool[2]=tempbool2;
  enemy::draw(dest);
}

int eWizzrobe::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(dying || clk<0 || hclk>0 || (superman && !(superman>1 && wpnId==wSBomb)))
    return 0;

  switch(wpnId)

  {
    case wPhantom:
    return 0;
    case wFire:
    case wArrow:
    case wWand:
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wFSparkle:
    return 0;
    case wBrang:
    if (dmisc2==2&&dmisc1==1) return 0;

    if (!current_item_power(itype_brang))
    {
      sfx(WAV_CHINK,pan(int(x)));
      return 1;
    }
    else
    {
      return enemy::takehit(wpnId,power,wpnx,wpny,wpnDir);
    }
    case wHookshot:
    if (dmisc2==2&&dmisc1==1) return 0;
    stunclk=160;
    break;
    case wMagic:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
    default:
    if ((dmisc2==2&&dmisc1==1)&&(wpnId!=wRefMagic)&&(wpnId!=wRefBeam)) return 0;
    hp-=power;
  }
  hclk=33;
  sfx(WAV_EHIT,pan(int(x)));
  return 1;
}

/*********************************/
/**********   Bosses   ***********/
/*********************************/

eDodongo::eDodongo(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  if (get_bit(quest_rules,qr_DODONGOCOLORFIX))
  {
    loadpalset(csBOSS,pSprite(spDIG));
    cs=csBOSS;
  }
  fading=fade_flash_die;
  //nets+5120;

  if (dir==down&&y>=128)
  {
    dir=up;
  }
  if (dir==right&&x>=208)
  {
    dir=left;
  }
}

bool eDodongo::animate(int index)
{
  if(dying)
  {
    return Dead();
  }
  if(clk2)                                                  // ate a bomb
  {
    if(--clk2==0)
      hp-=misc;                                             // store bomb's power in misc
  }
  else
    constant_walk(rate,homing,spw_clipright);
  hxsz = (dir<=down) ? 16 : 32;
  //    hysz = (dir>=left) ? 16 : 32;

  return enemy::animate(index);
}

void eDodongo::draw(BITMAP *dest)
{
  tile=o_tile;
  if(clk<0)
  {
    enemy::draw(dest);
    return;
  }

  update_enemy_frame();
  enemy::draw(dest);
  if (dummy_int[1]!=0) //additional tiles
  {
    tile+=dummy_int[1]; //second tile is previous tile
    xofs-=16;           //new xofs change
    enemy::draw(dest);
    xofs+=16;
  }

}

int eDodongo::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnDir=wpnDir;

  if(dying || clk<0 || clk2>0 || (superman && !(superman>1 && wpnId==wSBomb)))
    return 0;

  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wFire:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wFSparkle:
    return 0;
    case wLitBomb:
    case wLitSBomb:
    if(abs(wpnx-((dir==right)?x+16:x)) > 7 || abs(wpny-y) > 7)
      return 0;
    clk2=96;
    misc=power;
    if(wpnId==wLitSBomb)
      item_set=isSBOMB100;
    return 1;
    case wBomb:
    case wSBomb:
    if(abs(wpnx-((dir==right)?x+16:x)) > 8 || abs(wpny-y) > 8)
      return 0;
    stunclk=160;
    misc=wpnId;                                           // store wpnId
    return 1;
    case wSword:
    case wBeam:
    case wRefBeam:
    if(stunclk)
    {
      sfx(WAV_EHIT,pan(int(x)));
      hp=0;
      item_set = (misc==wSBomb) ? isSBOMB100 : isBOMB100;
      fading=0;                                           // don't flash
      return 1;
    }
    default:
    sfx(WAV_CHINK,pan(int(x)));
  }
  return 1;
}

void eDodongo::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

eDodongo2::eDodongo2(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  fading=fade_flash_die;
  //nets+5180;
  previous_dir=-1;

  if (dir==down&&y>=128)
  {
    dir=up;
  }
  if (dir==right&&x>=208)
  {
    dir=left;
  }
}

bool eDodongo2::animate(int index)
{
  if(dying)
  {
    return Dead();
  }
  if(clk2)                                                  // ate a bomb
  {
    if(--clk2==0)
      hp-=misc;                                             // store bomb's power in misc
  }
  else
    constant_walk(rate,homing,spw_clipbottomright);
  hxsz = (dir<=down) ? 16 : 32;
  hysz = (dir>=left) ? 16 : 32;
  hxofs=(dir>=left)?-8:0;
  hyofs=(dir<left)?-8:0;

  return enemy::animate(index);
}

void eDodongo2::draw(BITMAP *dest)
{
  if(clk<0)
  {
    enemy::draw(dest);
    return;
  }
  int tempx=xofs;
  int tempy=yofs;
  update_enemy_frame();
  enemy::draw(dest);
  tile+=dummy_int[1]; //second tile change
  xofs+=dummy_int[2]; //new xofs change
  yofs+=dummy_int[3]; //new yofs change
  enemy::draw(dest);
  xofs=tempx;
  yofs=tempy;
}

int eDodongo2::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnDir=wpnDir;

  if(dying || clk<0 || clk2>0 || superman)
    return 0;

  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wFire:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wFSparkle:
    return 0;
    case wLitBomb:
    case wLitSBomb:
    switch (dir)
    {
      case up:
      if(abs(wpnx-x) > 7 || abs(wpny-(y-8)) > 7)
        return 0;
      break;
      case down:
      if(abs(wpnx-x) > 7 || abs(wpny-(y+8)) > 7)
        return 0;
      break;
      case left:
      if(abs(wpnx-(x-8)) > 7 || abs(wpny-y) > 7)
        return 0;
      break;
      case right:
      if(abs(wpnx-(x+8)) > 7 || abs(wpny-y) > 7)
        return 0;
      break;
    }
    //          if(abs(wpnx-((dir==right)?x+8:(dir==left)?x-8:0)) > 7 || abs(wpny-((dir==down)?y+8:(dir==up)?y-8:0)) > 7)
    //            return 0;
    clk2=96;
    misc=power;
    if(wpnId==wLitSBomb)
      item_set=isSBOMB100;
    return 1;
    case wBomb:
    case wSBomb:
    switch (dir)
    {
      case up:
      if(abs(wpnx-x) > 7 || abs(wpny-(y-8)) > 7)
        return 0;
      break;
      case down:
      if(abs(wpnx-x) > 7 || abs(wpny-(y+8)) > 7)
        return 0;
      break;
      case left:
      if(abs(wpnx-(x-8)) > 7 || abs(wpny-y) > 7)
        return 0;
      break;
      case right:
      if(abs(wpnx-(x+8)) > 7 || abs(wpny-y) > 7)
        return 0;
      break;
    }
    stunclk=160;
    misc=wpnId;                                           // store wpnId
    return 1;
    case wSword:
    case wBeam:
    case wRefBeam:
    if(stunclk)
    {
      sfx(WAV_EHIT,pan(int(x)));
      hp=0;
      item_set = (misc==wSBomb) ? isSBOMB100 : isBOMB100;
      fading=0;                                           // don't flash
      return 1;
    }
    default:
    sfx(WAV_CHINK,pan(int(x)));
  }
  return 1;
}

void eDodongo2::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

eAquamentus::eAquamentus(fix X,fix Y,int Id,int Clk) : enemy((fix)176,(fix)64,Id,Clk)
{
  //these are here to bypass compiler warnings about unused arguments
  X=X;
  Y=Y;

  if (dmisc1)
  {
    x=64;
  }
  //nets+5940;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
  }
  else
  {
    if (dmisc1)
    {
      flip=1;
    }
  }

  yofs=playing_field_offset+1;
  clk3=32;
  clk2=0;
  dir=left;
}

bool eAquamentus::animate(int index)
{
  if(dying)
    return Dead(index);
  //  fbx=x+((id==eRAQUAM)?4:-4);
  fbx=x;
  /*
    if (get_bit(quest_rules,qr_NEWENEMYTILES)&&id==eLAQUAM)
    {
    fbx+=16;
    }
    */
  if(--clk3==0)
  {
//    addEwpn(fbx,y,z,ewFireball,0,d->wdp,up+1);
//    addEwpn(fbx,y,z,ewFireball,0,d->wdp,0);
//    addEwpn(fbx,y,z,ewFireball,0,d->wdp,down+1);
    addEwpn(fbx,y,z,wpn,0,wdp,up,getUID());
    addEwpn(fbx,y,z,wpn,0,wdp,8,getUID());
    addEwpn(fbx,y,z,wpn,0,wdp,down,getUID());
  }
  if(clk3<-80 && !(rand()&63))
  {
    clk3=32;
  }
  if(!((clk+1)&63))
  {
    int d2=(rand()%3)+1;
    if(d2>=left)
    {
      dir=d2;
    }
    if (dmisc1)
    {
      if(x<=40)
      {
        dir=right;
      }
      if(x>=104)
      {
        dir=left;
      }
    }
    else
    {
      if(x<=136)
      {
        dir=right;
      }
      if(x>=200)
      {
        dir=left;
      }
    }
  }
  if(clk>=-1 && !((clk+1)&7))
  {
    if(dir==left)
    {
      x-=1;
    }
    else
    {
      x+=1;
    }
  }

  return enemy::animate(index);
}

void eAquamentus::draw(BITMAP *dest)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    xofs=(dmisc1?-16:0);
    tile=o_tile+((clk&24)>>2)+(clk3>-32?(clk3>0?40:80):0);
    if (dying)
    {
      xofs=0;
      enemy::draw(dest);
    }
    else
    {
      drawblock(dest,15);
    }
  }
  else
  {
    int xblockofs=((dmisc1)?-16:16);
    xofs=0;
    if(clk<0 || dying)
    {
      enemy::draw(dest);
      return;
    }
    // face (0=firing, 2=resting)
    tile=o_tile+((clk3>0)?0:2); enemy::draw(dest);
    // tail (
    tile=o_tile+((clk&16)?1:3); xofs=xblockofs;  enemy::draw(dest);
    // body
    yofs+=16;
    xofs=0;  tile=o_tile+((clk&16)?20:22); enemy::draw(dest);
    xofs=xblockofs; tile=o_tile+((clk&16)?21:23); enemy::draw(dest);
    yofs-=16;
  }
}

bool eAquamentus::hit(weapon *w)
{
  switch(w->id)
  {
    case wBeam:
    case wRefBeam:
    case wMagic: hysz=32;
  }
  bool ret = (dying || hclk>0) ? false : sprite::hit(w);
  hysz=16;
  return ret;

}

int eAquamentus::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(dying || clk<0 || hclk>0 || (superman && !(superman>1 && wpnId==wSBomb)))
    return 0;
  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wFire:
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wFSparkle:
    return 0;
    case wHookshot:
    case wBrang:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
    default:
    hp-=power;
    hclk=33;
  }
  sfx(WAV_EHIT,pan(int(x)));
  sfx(WAV_GASP,pan(int(x)));
  return 1;
}

void eAquamentus::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

eGohma::eGohma(fix X,fix Y,int Id,int Clk) : enemy((fix)128,(fix)48,Id,0)
{
  //these are here to bypass compiler warnings about unused arguments
  X=X;
  Y=Y;
  Clk=Clk;

  hxofs=-16;
  hxsz=48;
  yofs=playing_field_offset+1;

  //nets+5340;
}

bool eGohma::animate(int index)
{
  if(dying)
    return Dead(index);
  if((clk&63)==0)
  {
    if(clk&64)
      dir^=1;
    else
      dir=rand()%3+1;
  }
  if((clk&63)==3)
  {
    switch (dmisc1)
    {
      case 0:
      case 1:
      addEwpn(x,y+2,z,wpn,1,wdp,8,getUID());
      break;
      case 2:
//      addEwpn(x,y+2,z,ewFireball,1,d->wdp,left+1);
//      addEwpn(x,y+2,z,ewFireball,1,d->wdp,0);
//      addEwpn(x,y+2,z,ewFireball,1,d->wdp,right+1);
      addEwpn(x,y+2,z,wpn,1,wdp,left,getUID());
      addEwpn(x,y+2,z,wpn,1,wdp,8,getUID());
      addEwpn(x,y+2,z,wpn,1,wdp,right,getUID());
      break;
    }
  }
  if ((dmisc1 == 3)&& clk3>=16 && clk3<116)
  {
    if (!(clk3%8))
    {
      addEwpn(x,y+4,z,ewFlame,0,4*DAMAGE_MULTIPLIER,255,getUID());
      sfx(WAV_FIRE,pan(int(x)));

      int i=Ewpns.Count()-1;
      weapon *ew = (weapon*)(Ewpns.spr(i));
      if (wpnsbuf[ewFLAME].frames>1)
      {
        ew->aframe=rand()%wpnsbuf[ewFLAME].frames;
        ew->tile+=ew->aframe;
      }
      for(int j=Ewpns.Count()-1; j>0; j--)
      {
        Ewpns.swap(j,j-1);
      }
    }
  }
  if(clk&1)
    move((fix)1);
  if(++clk3>=400)
    clk3=0;

  return enemy::animate(index);
}

void eGohma::draw(BITMAP *dest)
{
  tile=o_tile;
  if(clk<0 || dying)
  {
    enemy::draw(dest);
    return;
  }

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    // left side
    xofs=-16;
    flip=0;
    //      if(clk&16) tile=180;
    //      else { tile=182; flip=1; }
    tile+=(3*((clk&48)>>4));
    enemy::draw(dest);

    // right side
    xofs=16;
    //      tile=(180+182)-tile;
    tile=o_tile;
    tile+=(3*((clk&48)>>4))+2;
    enemy::draw(dest);

    // body
    xofs=0;
    tile=o_tile;
    //      tile+=(3*((clk&24)>>3))+2;
    if(clk3<16)
      tile+=7;
    else if(clk3<116)
        tile+=10;
      else if(clk3<132)
          tile+=7;
        else tile+=((clk3-132)&8)?4:1;
    enemy::draw(dest);

  }
  else
  {
    // left side
    xofs=-16;
    flip=0;
    if(!(clk&16)) { tile+=2; flip=1; }
    enemy::draw(dest);

    // right side
    tile=o_tile;
    xofs=16;
    if((clk&16)) tile+=2;
    //      tile=(180+182)-tile;
    enemy::draw(dest);

    // body
    tile=o_tile;
    xofs=0;
    if(clk3<16)
      tile+=4;
    else if(clk3<116)
        tile+=5;
      else if(clk3<132)
          tile+=4;
        else tile+=((clk3-132)&8)?3:1;
    enemy::draw(dest);

  }
}

int eGohma::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpny=wpny;

  if(dying || clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wFire:
    case wLitBomb:
    case wBomb:
    case wLitSBomb:
    case wSBomb:
    case wBait:
    case wWhistle:
    case wWind:
    case wSSparkle:
    case wFSparkle:
    case wPhantom:
    return 0;
    case wArrow:
    if(wpnDir==up && abs(int(x)-wpnx)<=8 && clk3>=16 && clk3<116)
    {
      if ((dmisc1<2)||
          ((dmisc1==2)&&(current_item_power(itype_arrow)>=4))||
          ((dmisc1==3)&&(current_item_power(itype_arrow)>=8)))
      {
        hp-=power;
        hclk=33;
        break;
        //        } else {
        //          return 0;
      }
    }
    // fall through
    default:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
  }
  sfx(WAV_EHIT,pan(int(x)));
  sfx(WAV_GASP,pan(int(x)));
  return 1;
}

void eGohma::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

eLilDig::eLilDig(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  count_enemy=(id==(id&0xFF));
  //nets+4360+(((id&0xFF)-eDIGPUP2)*40);
}

bool eLilDig::animate(int index)
{
  if(dying)
    return Dead(index);

  if(misc<=128)
  {
    if(!(++misc&31))
      step+=0.25;
  }
  variable_walk_8(rate,16,spw_floater);
  return enemy::animate(index);
}

void eLilDig::draw(BITMAP *dest)
{
  tile = o_tile;
  //    tile = 160;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);


  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
      flip=0;
      break;
      case l_up:                                            //d
      flip=0; tile+=4;
      break;
      case l_down:                                          //l
      flip=0; tile+=8;
      break;
      case left:                                            //r
      flip=0; tile+=12;
      break;
      case r_down:                                          //ul
      flip=0; tile+=20;
      break;
      case down:                                            //ur
      flip=0; tile+=24;
      break;
      case r_up:                                            //dl
      flip=0; tile+=28;
      break;
      case right:                                           //dr
      flip=0; tile+=32;
      break;
    }
    tile+=f2;
  }
  else
  {
    tile+=(clk>=6)?1:0;
  }

  enemy::draw(dest);
}

eBigDig::eBigDig(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  superman=1;
  hxofs=hyofs=-8;
  hxsz=hysz=32;
  hzsz=16; // hard to jump.
  //nets+4000;
}

bool eBigDig::animate(int index)
{
  if(dying)
    return Dead();
  switch(misc)
  {
    case 0: variable_walk_8(2,16,spw_floater,-8,-16,23,23); break;
    case 1: ++misc; break;
    case 2:
    /*if(id==eDIG3)
    {
      guys.add(new eLilDig(x,y,eDIGPUP2+0x1000,-15));
      guys.add(new eLilDig(x,y,eDIGPUP3+0x1000,-15));
      guys.add(new eLilDig(x,y,eDIGPUP4+0x1000,-15));
    }
    else
    {
      guys.add(new eLilDig(x,y,eDIGPUP1+0x1000,-15));
    }*/
    for(int i=0;i<dmisc5;i++)
	{
	  //guys.add(new eLilDig(x,y,dmisc1+0x1000,-15));
	  addenemy(x,y,dmisc1+0x1000,-15);
	}
	for(int i=0;i<dmisc6;i++)
	{
	  //guys.add(new eLilDig(x,y,dmisc2+0x1000,-15));
	  addenemy(x,y,dmisc2+0x1000,-15);
	}
	for(int i=0;i<dmisc7;i++)
	{
	  //guys.add(new eLilDig(x,y,dmisc3+0x1000,-15));
	  addenemy(x,y,dmisc3+0x1000,-15);
	}
	for(int i=0;i<dmisc8;i++)
	{
	  //guys.add(new eLilDig(x,y,dmisc4+0x1000,-15));
	  addenemy(x,y,dmisc4+0x1000,-15);
	}

    stop_sfx(bgsfx);
    sfx(WAV_GASP,pan(int(x)));
    return true;
  }

  return enemy::animate(index);
}

void eBigDig::draw(BITMAP *dest)
{
  tile = o_tile;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
      flip=0;
      break;
      case l_up:                                            //d
      flip=0; tile+=8;
      break;
      case l_down:                                          //l
      flip=0; tile+=40;
      break;
      case left:                                            //r
      flip=0; tile+=48;
      break;
      case r_down:                                          //ul
      flip=0; tile+=80;
      break;
      case down:                                            //ur
      flip=0; tile+=88;

      break;
      case r_up:                                            //dl
      flip=0; tile+=120;
      break;
      case right:                                           //dr
      flip=0; tile+=128;
      break;
    }
    tile+=(f2*2);
  }
  else
  {
    tile+=(f2)?0:2;
    flip=(clk&1)?1:0;
  }
  xofs-=8; yofs-=8;
  drawblock(dest,15);
  xofs+=8; yofs+=8;
}

int eBigDig::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  power=power;
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(wpnId==wWhistle && misc==0)
    misc=1;
  return 0;
}

eGanon::eGanon(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  hxofs=hyofs=8;
  hzsz=16; //can't be jumped.
  clk2=70;
  misc=-1;
  mainguy=!getmapflag();
}

bool eGanon::animate(int index)
{
  if(dying)

    return Dead();

  switch(misc)
  {
    case -1: misc=0;
    case 0:
    if(++clk2>72 && !(rand()&3))
    {
      addEwpn(x,y,z,wpn,1,wdp,dir,getUID());
      clk2=0;
    }
    Stunclk=0;
    constant_walk(rate,homing,spw_none);
    break;

    case 1:
    case 2:
    if(--Stunclk<=0)
    {
      int r=rand();
      if(r&1)
      {
        y=96;
        if(r&2)
          x=160;
        else
          x=48;
        if(tooclose(x,y,48))
          x=208-x;
      }
      loadpalset(csBOSS,pSprite(d->bosspal));
      misc=0;
    }
    break;
    case 3:
    if(hclk>0)
      break;
    misc=4;
    clk=0;
    hxofs=1000;
    loadpalset(9,pSprite(spPILE));
    music_stop();
    stop_sfx(WAV_ROAR);
    sfx(WAV_GASP,pan(int(x)));
    sfx(WAV_GANON);
    items.add(new item(x+8,y+8,(fix)0,iPile,ipDUMMY,0));
    break;

    case 4:
    if(clk>=80)
    {
      misc=5;
      if(getmapflag())
      {
        game->lvlitems[dlevel]|=liBOSS;
        //play_DmapMusic();
        playLevelMusic();
        return true;
      }
      sfx(WAV_CLEARED);
      items.add(new item(x+8,y+8,(fix)0,iBigTri,ipBIGTRI,0));
    }
    break;
  }
  return enemy::animate(index);
}


int eGanon::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  switch(misc)
  {
    case 0:
    if(wpnId!=wSword)
      return 0;
    hp-=power;
    if(hp>0)
    {
      misc=1;
      Stunclk=64;
    }
    else
    {
      loadpalset(csBOSS,pSprite(spBROWN));
      misc=2;
      Stunclk=284;
      hp=guysbuf[id].hp;                              //16*DAMAGE_MULTIPLIER;
    }
    sfx(WAV_EHIT,pan(int(x)));
    sfx(WAV_GASP,pan(int(x)));
    return 1;

    case 2:
    if(wpnId!=wArrow || current_item_power(itype_arrow)<4)
      return 0;
    misc=3;
    hclk=81;
    loadpalset(9,pSprite(spBROWN));
    return 1;
  }
  return 0;
}

void eGanon::draw(BITMAP *dest)
{
  switch(misc)
  {
    case 0:
    if((clk&3)==3)
      tile=(rand()%5)*2+o_tile;
    if(db!=999)
      break;
    case 2:
    if(Stunclk<64 && (Stunclk&1))
      break;
    case -1:
    tile=o_tile;
    //fall through
    case 1:
    case 3:
    drawblock(dest,15);
    break;
    case 4:
    draw_guts(dest);
    draw_flash(dest);
    break;
  }
}

void eGanon::draw_guts(BITMAP *dest)
{
  int c = zc_min(clk>>3,8);
  tile = clk<24 ? 74 : 75;
  overtile16(dest,tile,x+8,y+c+playing_field_offset,9,0);
  overtile16(dest,tile,x+8,y+16-c+playing_field_offset,9,0);
  overtile16(dest,tile,x+c,y+8+playing_field_offset,9,0);
  overtile16(dest,tile,x+16-c,y+8+playing_field_offset,9,0);
  overtile16(dest,tile,x+c,y+c+playing_field_offset,9,0);
  overtile16(dest,tile,x+16-c,y+c+playing_field_offset,9,0);
  overtile16(dest,tile,x+c,y+16-c+playing_field_offset,9,0);
  overtile16(dest,tile,x+16-c,y+16-c+playing_field_offset,9,0);
}

void eGanon::draw_flash(BITMAP *dest)
{

  int c = clk-(clk>>2);
  cs = (frame&3)+6;
  overtile16(dest,194,x+8,y+8-clk+playing_field_offset,cs,0);
  overtile16(dest,194,x+8,y+8+clk+playing_field_offset,cs,2);
  overtile16(dest,195,x+8-clk,y+8+playing_field_offset,cs,0);
  overtile16(dest,195,x+8+clk,y+8+playing_field_offset,cs,1);
  overtile16(dest,196,x+8-c,y+8-c+playing_field_offset,cs,0);
  overtile16(dest,196,x+8+c,y+8-c+playing_field_offset,cs,1);
  overtile16(dest,196,x+8-c,y+8+c+playing_field_offset,cs,2);
  overtile16(dest,196,x+8+c,y+8+c+playing_field_offset,cs,3);
}

void getBigTri(int id2)
{
  /*
    *************************
    * BIG TRIFORCE SEQUENCE *
    *************************
    0 BIGTRI out, WHITE flash in
    4 WHITE flash out, PILE cset white
    8 WHITE in
    ...
    188 WHITE out
    191 PILE cset red
    200 top SHUTTER opens
    209 bottom SHUTTER opens
    */
  sfx(itemsbuf[id2].playsound);
  guys.clear();

  draw_screen(tmpscr, 0, 0);

  for(int f=0; f<24*8 && !Quit; f++)
  {
    if(f==4)
    {
      for(int i=1; i<16; i++)
      {
        RAMpal[CSET(9)+i]=_RGB(63,63,63);
      }
    }
    if((f&7)==0)
    {
      for(int cs=2; cs<5; cs++)
      {
        for(int i=1; i<16; i++)
        {
          RAMpal[CSET(cs)+i]=_RGB(63,63,63);
        }
      }
      refreshpal=true;
    }
    if((f&7)==4)
    {
      loadlvlpal(DMaps[currdmap].color);
    }
    if(f==191)
    {
      loadpalset(9,pSprite(spPILE));
    }
    advanceframe(true);
  }

  //play_DmapMusic();
  playLevelMusic();
  game->lvlitems[dlevel]|=liBOSS;
  setmapflag();
}

/**********************************/
/***  Multiple-Segment Enemies  ***/
/**********************************/

eMoldorm::eMoldorm(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  x=128; y=48;
  dir=(rand()&7)+8;
  superman=1;
  fading=fade_invisible;
  hxofs=1000;
  segcnt=clk;
  clk=0;
  id=guys.Count();
  yofs=playing_field_offset;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    tile=nets+1220;
  }
  else
  {
    tile=57;
  }
}

bool eMoldorm::animate(int index)
{

  if(clk2)
  {
    if(--clk2 == 0)
    {
      never_return(index);
      leave_item();
      return true;
    }
  }
  else
  {
    constant_walk_8(rate,spw_floater);
    misc=dir;

    for(int i=index+1; i<index+segcnt+1; i++)
    {
      ((enemy*)guys.spr(i))->o_tile=tile;
      if ((i==index+segcnt)&&(i!=index+1))                  //tail
      {
        ((enemy*)guys.spr(i))->dummy_int[1]=2;
      }
      else
      {
        ((enemy*)guys.spr(i))->dummy_int[1]=1;
      }
      if (i==index+1)                                       //head
      {
        ((enemy*)guys.spr(i))->dummy_int[1]=0;
      }
      if(((enemy*)guys.spr(i))->hp <= 0)
      {
        for(int j=i; j<index+segcnt; j++)
        {
          zc_swap(((enemy*)guys.spr(j))->hp,((enemy*)guys.spr(j+1))->hp);
          zc_swap(((enemy*)guys.spr(j))->hclk,((enemy*)guys.spr(j+1))->hclk);
        }
        ((enemy*)guys.spr(i))->hclk=33;
        --segcnt;
      }
    }
    if(segcnt==0)
    {
      clk2=19;

      x=guys.spr(index+1)->x;
      y=guys.spr(index+1)->y;
    }
  }
  return false;
}

esMoldorm::esMoldorm(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  x=128; y=48;
  yofs=playing_field_offset;
  hyofs=4;
  hxsz=hysz=8;
  hxofs=1000;
  mainguy=count_enemy=false;
  parentclk = 0;
  item_set=0;
}

bool esMoldorm::animate(int index)
{
  if(dying)
    return Dead();
  if(clk>=0)
  {
    hxofs=4;
	step=((enemy*)guys.spr(index-1))->step;
    if(parentclk == 0)
    {
	  misc=dir;
      dir=((enemy*)guys.spr(index-1))->misc;
	  //do alignment, as in parent's anymation :-/ -DD
	  x.v&=0xFFFF0000;
	  y.v&=0xFFFF0000;
    }
	parentclk=(parentclk+1)%((int)(8.0/step));
    if (!watch)
    {
      sprite::move(step);
    }
  }
  return enemy::animate(index);
}

int esMoldorm::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(enemy::takehit(wpnId,power,wpnx,wpny,wpnDir))
    return (wpnId==wSBomb) ? 1 : 2;                         // force it to wait a frame before checking sword attacks again
  return 0;
}

void esMoldorm::draw(BITMAP *dest)
{
  tile=o_tile;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    tile+=dummy_int[1]*40;
    switch (dir-8)                                          //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
      flip=0;
      break;
      case l_up:                                            //d
      flip=0; tile+=4;
      break;
      case l_down:                                          //l
      flip=0; tile+=8;
      break;
      case left:                                            //r
      flip=0; tile+=12;
      break;
      case r_down:                                          //ul
      flip=0; tile+=20;
      break;
      case down:                                            //ur
      flip=0; tile+=24;
      break;
      case r_up:                                            //dl
      flip=0; tile+=28;
      break;
      case right:                                           //dr
      flip=0; tile+=32;

      break;
    }
    tile+=f2;
  }

  if(clk>=0)
    enemy::draw(dest);
}

void esMoldorm::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

eLanmola::eLanmola(fix X,fix Y,int Id,int Clk) : eBaseLanmola(X,Y,Id,Clk)
{
  x=64; y=80;
  dir=up;
  superman=1;
  fading=fade_invisible;
  hxofs=1000;
  segcnt=clk;
  clk=0;
  id=guys.Count();
  //set up move history
  for(int i=0; i < (1<<dmisc2); i++)
	  prevState.push_back(std::pair<std::pair<fix, fix>, int>( std::pair<fix,fix>(x,y), dir ));
}

bool eLanmola::animate(int index)
{
  if(clk2)
  {
    if(--clk2 == 0)
    {
      leave_item();
      return true;
    }
    return false;
  }


  //this animation style plays ALL KINDS of havoc on the Lanmola segments, since it causes
  //the direction AND x,y position of the lanmola to vary in uncertain ways.
  //I've added a complete movement history to this enemy to compensate -DD
  constant_walk(rate,homing,spw_none);
  prevState.pop_front();
  prevState.push_front(std::pair<std::pair<fix, fix>, int>(std::pair<fix, fix>(x,y), dir));

  for(int i=index+1; i<index+segcnt+1; i++)
  {
    ((enemy*)guys.spr(i))->o_tile=o_tile;
    if ((i==index+segcnt)&&(i!=index+1))
    {
      ((enemy*)guys.spr(i))->dummy_int[1]=1;                //tail
    }
    else
    {
      ((enemy*)guys.spr(i))->dummy_int[1]=0;
    }
    if(((enemy*)guys.spr(i))->hp <= 0)
    {
      for(int j=i; j<index+segcnt; j++)
      {
        zc_swap(((enemy*)guys.spr(j))->hp,((enemy*)guys.spr(j+1))->hp);
        zc_swap(((enemy*)guys.spr(j))->hclk,((enemy*)guys.spr(j+1))->hclk);
      }
      ((enemy*)guys.spr(i))->hclk=33;
      --segcnt;
    }
  }
  if(segcnt==0)
  {
    clk2=19;
    x=guys.spr(index+1)->x;
    y=guys.spr(index+1)->y;
    setmapflag(mTMPNORET);
  }
  return enemy::animate(index);
}

esLanmola::esLanmola(fix X,fix Y,int Id,int Clk) : eBaseLanmola(X,Y,Id,Clk)
{
  x=64; y=80;
  crate=dmisc3;
  hxofs=1000;
  hxsz=8;
  mainguy=false;
  count_enemy=(id<0x2000)?true:false;
  item_set=0;
  //set up move history
  for(int i=0; i < (1<<dmisc2); i++)
	  prevState.push_back(std::pair<std::pair<fix, fix>, int>( std::pair<fix,fix>(x,y), dir ));

}

bool esLanmola::animate(int index)
{
  if(dying)
  {
    xofs=0;
    return Dead();
  }
  if(clk>=0)
  {
    hxofs=4;
	if (!watch)
    {
		std::pair<std::pair<fix, fix>, int> newstate = ((eBaseLanmola*)guys.spr(index-1))->prevState.front();
		prevState.pop_front();
		prevState.push_back(newstate);
		x = newstate.first.first;
		y = newstate.first.second;
		dir = newstate.second;
    }
  }
  return enemy::animate(index);
}

int esLanmola::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  if(enemy::takehit(wpnId,power,wpnx,wpny,wpnDir))
    return (wpnId==wSBomb) ? 1 : 2;                         // force it to wait a frame before checking sword attacks again
  return 0;
}

void esLanmola::draw(BITMAP *dest)
{
  tile=o_tile;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (id>=0x2000)
    {
      tile+=20;
      if (dummy_int[1]==1)
      {
        tile+=20;
      }
    }
    switch (dir)
    {
      case up:
      flip=0;
      break;
      case down:
      flip=0; tile+=4;
      break;
      case left:
      flip=0; tile+=8;
      break;
      case right:
      flip=0; tile+=12;
      break;
    }
    tile+=f2;
  }
  else
  {
    if (id>=0x2000)
    {
      tile+=1;
    }
  }

  if(clk>=0)
    enemy::draw(dest);
}

eManhandla::eManhandla(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,0)
{
  //these are here to bypass compiler warnings about unused arguments
  Clk=Clk;

  superman=1;
  dir=(rand()&7)+8;
  armcnt=dmisc2?8:4;//((id==eMANHAN)?4:8);
  for(int i=0; i<armcnt; i++)
    arm[i]=i;
  fading=fade_blue_poof;
  //nets+4680;
  adjusted=false;
}

bool eManhandla::animate(int index)
{
  if(dying)
    return Dead(index);


  // check arm status, move dead ones to end of group
  for(int i=0; i<armcnt; i++)
  {
    if (!adjusted)
    {
      if (!dmisc2)
      {
        ((enemy*)guys.spr(index+i+1))->o_tile=o_tile+40;
      }
      else
      {
        ((enemy*)guys.spr(index+i+1))->o_tile=o_tile+160;
      }
    }
    if(((enemy*)guys.spr(index+i+1))->dying)
    {
      for(int j=i; j<armcnt-1; j++)
      {
        zc_swap(arm[j],arm[j+1]);
        guys.swap(index+j+1,index+j+2);

      }
      --armcnt;
    }
  }
  adjusted=true;
  // move or die
  if(armcnt==0)
    hp=0;
  else
  {
    step=(((!dmisc2)?5:9)-armcnt)*0.5;
    int dx1=0, dy1=-8, dx2=15, dy2=15;
    if (!dmisc2)
    {
      for(int i=0; i<armcnt; i++)
      {
        switch(arm[i])
        {
          case 0: dy1=-24; break;
          case 1: dy2=31;  break;
          case 2: dx1=-16; break;
          case 3: dx2=31;  break;
        }
      }
    }
    else
    {
      dx1=-8, dy1=-16, dx2=23, dy2=23;
      for(int i=0; i<armcnt; i++)
      {
        switch(arm[i]&3)
        {
          case 0: dy1=-32; break;
          case 1: dy2=39;  break;
          case 2: dx1=-24; break;
          case 3: dx2=39;  break;
        }
      }
    }
    variable_walk_8(rate,16,spw_floater,dx1,dy1,dx2,dy2);
    for(int i=0; i<armcnt; i++)
    {
      fix dx=(fix)0,dy=(fix)0;
      if (!dmisc2)
      {
        switch(arm[i])
        {
          case 0: dy=-16; break;
          case 1: dy=16;  break;
          case 2: dx=-16; break;
          case 3: dx=16;  break;
        }
      }
      else
      {
        switch(arm[i])
        {
          case 0: dy=-24; dx=-8; break;
          case 1: dy=24;  dx=8;  break;
          case 2: dx=-24; dy=8; break;
          case 3: dx=24;  dy=-8;  break;
          case 4: dy=-24; dx=8; break;
          case 5: dy=24;  dx=-8;  break;
          case 6: dx=-24; dy=-8; break;
          case 7: dx=24;  dy=8;  break;
        }
      }
      guys.spr(index+i+1)->x = x+dx;
      guys.spr(index+i+1)->y = y+dy;
    }
  }
  return enemy::animate(index);
}

void eManhandla::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

int eManhandla::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  power=power;
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(dying)
    return 0;
  switch(wpnId)
  {
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wFire:
    case wWind:
    case wSSparkle:
    case wFSparkle:
    case wPhantom:
    return 0;
    case wHookshot:
    case wBrang: sfx(WAV_CHINK,pan(int(x))); break;
    default:     sfx(WAV_EHIT,pan(int(x)));

  }
  return 1;
}

void eManhandla::draw(BITMAP *dest)
{
  tile=o_tile;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    if (!dmisc2)
    {
      switch (dir-8)                                        //directions get screwed up after 8.  *shrug*
      {
        case up:                                            //u
        flip=0;
        break;
        case l_up:                                          //d
        flip=0; tile+=4;
        break;
        case l_down:                                        //l
        flip=0; tile+=8;
        break;
        case left:                                          //r
        flip=0; tile+=12;
        break;
        case r_down:                                        //ul
        flip=0; tile+=20;
        break;
        case down:                                          //ur
        flip=0; tile+=24;
        break;
        case r_up:                                          //dl
        flip=0; tile+=28;
        break;
        case right:                                         //dr
        flip=0; tile+=32;
        break;
      }
      tile+=f2;
      enemy::draw(dest);
    }                                                       //manhandla 2, big body
    else
    {

      switch (dir-8)                                        //directions get screwed up after 8.  *shrug*
      {
        case up:                                            //u
        flip=0;
        break;
        case l_up:                                          //d
        flip=0; tile+=8;
        break;
        case l_down:                                        //l
        flip=0; tile+=40;
        break;
        case left:                                          //r
        flip=0; tile+=48;
        break;
        case r_down:                                        //ul
        flip=0; tile+=80;
        break;
        case down:                                          //ur
        flip=0; tile+=88;
        break;
        case r_up:                                          //dl
        flip=0; tile+=120;
        break;
        case right:                                         //dr
        flip=0; tile+=128;
        break;
      }
      tile+=(f2*2);
      xofs-=8;  yofs-=8;  drawblock(dest,15);
      xofs+=8;  yofs+=8;
    }
  }
  else
  {
    if (!dmisc2)
    {
      enemy::draw(dest);
    }
    else
    {
      xofs-=8;  yofs-=8;  enemy::draw(dest);
      xofs+=16;           enemy::draw(dest);
      yofs+=16; enemy::draw(dest);
      xofs-=16;           enemy::draw(dest);
      xofs+=8;  yofs-=8;
    }
  }
}

esManhandla::esManhandla(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  id=misc=clk;
  dir = clk & 3;
  clk=0;
  mainguy=count_enemy=false;
  dummy_bool[0]=false;
  item_set=0;
}

bool esManhandla::animate(int index)

{
  if(dying)
    return Dead();
  if(--clk2<=0)
  {
    clk2=unsigned(rand())%5+5;
    clk3^=1;
  }
  if(!(rand()&127))
  {
    addEwpn(x,y,z,wpn,1,wdp,dir,getUID());
  }
  return enemy::animate(index);
}

int esManhandla::takehit(int wpnId,int power,int ,int,int)
{
  if(dying || hclk>0)
    return 0;
  switch(wpnId)
  {
    case wLitBomb:
    case wLitSBomb:
    case wBait:
    case wWhistle:
    case wFire:
    case wWind:
    case wSSparkle:
    case wFSparkle:
    case wPhantom:
    return 0;
    case wHookshot:
    case wBrang: sfx(WAV_CHINK,pan(int(x))); break;
    default:
    hp-=power;
    hclk=33;
    sfx(WAV_EHIT,pan(int(x)));
    sfx(WAV_GASP,pan(int(x)));
  }
  return 1;
}

void esManhandla::draw(BITMAP *dest)
{
  tile=o_tile;
  int f2=get_bit(quest_rules,qr_NEWENEMYTILES)?
    (clk/(frate/4)):((clk>=(frate>>1))?1:0);
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch(misc&3)
    {
      case up:             break;
      case down:  tile+=4; break;
      case left:  tile+=8; break;
      case right: tile+=12; break;
    }
    tile+=f2;
  }
  else
  {
    switch(misc&3)
    {
      case down:  flip=2;
      case up:    tile=(clk3)?188:189; break;
      case right: flip=1;
      case left:  tile=(clk3)?186:187; break;
    }
  }
  enemy::draw(dest);
}

eGleeok::eGleeok(fix,fix,int Id,int Clk) : enemy((fix)120,(fix)48,Id,Clk)
{
  hzsz = 32; // can't be jumped.
  flameclk=0;
  misc=clk;                                                 // total head count
  clk3=clk;                                                 // live head count
  clk=0;
  clk2=60;                                                  // fire ball clock
                                                            //    hp=(guysbuf[eGLEEOK2+(misc-2)].misc2)*(misc-1)*DAMAGE_MULTIPLIER+guysbuf[eGLEEOK2+(misc-2)].hp;
  hp=(guysbuf[id].misc2)*(misc-1)*DAMAGE_MULTIPLIER+guysbuf[id].hp;
  hxofs=4;
  hxsz=8;
  //    frate=17*4;
  fading=fade_blue_poof;
  //nets+5420;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    necktile=o_tile+8;
    if (dmisc3)
    {
      necktile+=8;
    }
  }
}

bool eGleeok::animate(int index)
{
  if(dying)
    return Dead(index);

  //fix for the "kill all enemies" item
  if (hp==-1000)
  {
    for (int i=0; i<clk3; ++i)
    {
      ((enemy*)guys.spr(index+i+1))->hp=1;                   // re-animate each head,
      ((enemy*)guys.spr(index+i+1))->misc = -1;              // disconnect it,
      ((enemy*)guys.spr(index+i+1))->animate(index+i+1);     // let it animate one frame,
      ((enemy*)guys.spr(index+i+1))->hp=-1000;               // and kill it for good
    }
    clk3=0;
    for(int i=0; i<misc; i++)
      ((enemy*)guys.spr(index+i+1))->misc = -2;             // give the signal to disappear
  }

  for(int i=0; i<clk3; i++)
  {
    enemy *head = ((enemy*)guys.spr(index+i+1));
    head->dummy_int[1]=necktile;
    if (dmisc3)
    {
      head->dummy_bool[0]=true;
    }
    if(head->hclk)
    {
      if(hclk==0)
      {
        hp -= 1000 - head->hp;
        hclk = 33;
        sfx(WAV_GASP);
        sfx(WAV_EHIT,pan(int(head->x)));
      }
      head->hp = 1000;
      head->hclk = 0;
    }
  }

  if(hp<=(guysbuf[id].misc2)*(clk3-1)*DAMAGE_MULTIPLIER)
  {
    ((enemy*)guys.spr(index+clk3))->misc = -1;              // give signal to fly off
    hp=(guysbuf[id].misc2)*(--clk3)*DAMAGE_MULTIPLIER;
  }

  if (!dmisc3)
  {
    if(++clk2>72 && !(rand()&3))
    {
      int i=rand()%misc;
      enemy *head = ((enemy*)guys.spr(index+i+1));
      addEwpn(head->x,head->y,head->z,wpn,1,wdp,dir,getUID());
      clk2=0;
    }
  }
  else
  {
    if(++clk2>100 && !(rand()&3))
    {
      enemy *head = ((enemy*)guys.spr(rand()%misc+index+1));
      head->timer=rand()%50+50;
      clk2=0;
    }
  }

  if(hp<=0)
  {
    for(int i=0; i<misc; i++)
      ((enemy*)guys.spr(index+i+1))->misc = -2;             // give the signal to disappear
    never_return(index);
  }
  return enemy::animate(index);
}

int eGleeok::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnId=wpnId;
  power=power;
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  return 0;
}

void eGleeok::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

void eGleeok::draw(BITMAP *dest)
{
  tile=o_tile;

  if(dying)
  {
    enemy::draw(dest);
    return;
  }

  int f=clk/17;

  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    // body
    xofs=-8; yofs=32;
    switch(f)

    {
      case 0: tile+=0; break;
      case 1: tile+=2; break;
      case 2: tile+=4; break;
      default: tile+=6; break;
    }
  }
  else
  {
    // body
    xofs=-8; yofs=32;
    switch(f)
    {
      case 0: tile+=0; break;
      case 2: tile+=4; break;
      default: tile+=2; break;
    }
  }
  enemy::drawblock(dest,15);
}

void eGleeok::draw2(BITMAP *dest)
{
  // the neck stub
  tile=necktile;
  xofs=0;
  yofs=playing_field_offset;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    tile+=((clk&24)>>3);
  }
  else
  {
    tile=145;
  }
  /*
    if(hp>0 && !dont_draw())
    sprite::draw(dest);
    */
  if(hp > 0 && !dont_draw())
  {
    if ((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet)))
      sprite::drawcloaked(dest);
    else
      sprite::draw(dest);
  }
}

esGleeok::esGleeok(fix X,fix Y,int Id,int Clk, sprite * prnt) : enemy(X,Y,Id,Clk), parent(prnt)
{
	xoffset=0;
	yoffset=(fix)22;
  dummy_bool[0]=false;
  timer=0;
  /*  fixing */
  hp=1000;
  step=1;
  item_set=0;
  //x=120; y=70;
  x = xoffset+parent->x;
  y = yoffset+parent->y;
  hxofs=4;
  hxsz=8;
  yofs=playing_field_offset;
  clk2=clk;                                                 // how long to wait before moving first time
  clk=0;
  mainguy=count_enemy=false;
  dir=rand();
  clk3=((dir&2)>>1)+2;                                      // left or right
  dir&=1;                                                   // up or down
  for(int i=0; i<4; i++)
  {
	nxoffset[i] = 0;
	nyoffset[i] = 0;
	nx[i] = ( ( ( (i*x) + (4-i)*((int)parent->x)) ) >>2 );
    ny[i] = ( ( ( (i*y) + (4-i)*((int)parent->y)) ) >>2 );
  }
  //TODO compatibility? -DD
  /*
  for(int i=0; i<4; i++)
  {
    nx[i]=124;
    ny[i]=i*6+48;
  }*/
}

bool esGleeok::animate(int index)
{
	if(misc == 0)
	{
		x = (xoffset+parent->x);
		y = (yoffset+parent->y);
		for(int i=0; i<4; i++)
		{
			nx[i] = ( ( ( (i*x) + (4-i)*((int)parent->x)) ) >>2 ) + 3 + nxoffset[i];
			ny[i] = ( ( ( (i*y) + (4-i)*((int)parent->y)) ) >>2 ) + nyoffset[i];
		}
	}

  //  set up the head tiles
  headtile=nets+5588;                                       //5580, actually.  must adjust for direction later on
  if (dummy_bool[0])                                        //if this is a flame gleeok
  {
    headtile+=180;
  }
  flyingheadtile=headtile+60;

  //  set up the neck tiles
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    necktile=dummy_int[1]+((clk&24)>>3);
  }
  else
  {
    necktile=145;
  }
  //    ?((dummy_bool[0])?(nets+4052+(16+((clk&24)>>3))):(nets+4040+(8+((clk&24)>>3)))):145)

  switch(misc)
  {
    case 0:                                                 // live head
                                                            //  set up the attached head tiles
    if (get_bit(quest_rules,qr_NEWENEMYTILES))
    {
      tile=headtile;
      tile+=((clk&24)>>3);
      /*
        if (dummy_bool[0]) {
        tile+=1561;
        }
        */
    }
    else
    {
      tile=146;
    }
    if(++clk2>=0 && !(clk2&3))
    {
      //if(y<=56) dir=down;
		if(y<= (int)parent->y + 8) dir=down;
      //if(y>=80) dir=up;
		if(y>= (int)parent->y + 32) dir = up;
      //if(y<=58 && !(rand()&31))                           // y jig
		if(y<= (int)parent->y + 10 && !(rand()&31))
      {
        dir^=1;
      }
	  fix tempx = x;
	  fix tempy = y;

      sprite::move(step);
	  xoffset += (x-tempx);
	  yoffset += (y-tempy);
      if(clk2>=4)
      {
        clk3^=1;
        clk2=-4;
      }
      else
      {
        //if(x<=96)
	    if(x <= (int)parent->x-24)
        {
          clk3=right;
        }
        //if(x>=144)
		if(x >= (int)parent->x+24)
        {
          clk3=left;
        }
        //if(y<=72 && !(rand()&15))
		if(y <= (int)parent->y+24 && !(rand()&15))
        {
          clk3^=1;                                        // x jig
        }
        else
        {
          //if(y<=64 && !(rand()&31))
			if(y<=(int)parent->y+16 && !(rand()&31))
          {
            clk3^=1;                                      // x switch back
          }
          clk2=-4;
        }
      }
      zc_swap(dir,clk3);
	  tempx = x;
	  tempy = y;
      sprite::move(step);
	  xoffset += (x-tempx);
	  yoffset += (y-tempy);
      zc_swap(dir,clk3);

      for(int i=1; i<4; i++)
      {
        nxoffset[i] = (rand()%3);
        nyoffset[i] = (rand()%3);
      }
    }
    break;
    case 1:                                                 // flying head
    if(clk>=0)

    {
      variable_walk_8(2,9,spw_floater);
    }

    break;
    // the following are messages sent from the main guy...
    case -1:                                                // got chopped off
    {
      misc=1;
      superman=1;
      hxofs=xofs=0;
      hxsz=16;
      cs=8;
      clk=-24;
      clk2=40;
      dir=(rand()&7)+8;
      step=8.0/9.0;
    } break;
    case -2:                                                // the big guy is dead
    return true;
  }

  if (timer)
  {
    if (!(timer%8))
    {
      addEwpn(x,y+4,z,ewFlame,0,4*DAMAGE_MULTIPLIER,255,getUID());
      sfx(WAV_FIRE,pan(int(x)));

      int i=Ewpns.Count()-1;
      weapon *ew = (weapon*)(Ewpns.spr(i));
      if (wpnsbuf[ewFLAME].frames>1)
      {
        ew->aframe=rand()%wpnsbuf[ewFLAME].frames;
        ew->tile+=ew->aframe;
      }
      for(int j=Ewpns.Count()-1; j>0; j--)
      {
        Ewpns.swap(j,j-1);
      }
    }
    --timer;
  }

  return enemy::animate(index);
}

int esGleeok::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
    case wWand:
    case wBeam:
    case wRefBeam:
    case wHammer:
    case wSword:
    hp-=power;
    hclk=33;
    return 2;                                             // force it to wait a frame before checking sword attacks again
  }
  return 0;
}

void esGleeok::draw(BITMAP *dest)
{
  switch(misc)
  {
    case 0:                                                 //neck
    if(!dont_draw())
    {
      for(int i=1; i<4; i++)                              //draw the neck
      {
        if (get_bit(quest_rules,qr_NEWENEMYTILES))
        {
          if ((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet)))
            overtilecloaked16(dest,necktile+(i*40),nx[i]-4,ny[i]+playing_field_offset,0);
          else
            overtile16(dest,necktile+(i*40),nx[i]-4,ny[i]+playing_field_offset,cs,0);
        }
        else
        {
          if ((tmpscr->flags3&fINVISROOM)&& !(current_item(itype_amulet)))
            overtilecloaked16(dest,necktile,nx[i]-4,ny[i]+playing_field_offset,0);
          else
            overtile16(dest,necktile,nx[i]-4,ny[i]+playing_field_offset,cs,0);
        }
      }
    }

    break;

    case 1:                                                 //flying head
    if (get_bit(quest_rules,qr_NEWENEMYTILES))
    {
      tile=flyingheadtile;
      tile+=((clk&24)>>3);
      /*
        if (dummy_bool[0]) {
        tile+=1561;
        }
        */
      break;
    }
    else
    {
      tile=(clk&1)?147:148;
      break;
    }
  }
}

void esGleeok::draw2(BITMAP *dest)
{
  enemy::draw(dest);
}

ePatra::ePatra(fix X,fix Y,int Id,int Clk) : enemy((fix)128,(fix)48,Id,Clk)
{
  //these are here to bypass compiler warnings about unused arguments
  X=X;
  Y=Y;

  adjusted=false;
  dir=(rand()&7)+8;
  step=0.25;
  //flycnt=8; flycnt2=0;
  //if ((id==ePATRAL2)||(id==ePATRAL3))
  //{
    //flycnt2=8;
    //hp=22*DAMAGE_MULTIPLIER;
  //}
  flycnt=dmisc1;
  flycnt2=dmisc2;
  loopcnt=0;
  //nets+3440;
}

bool ePatra::animate(int index)
{
  if(dying)
    return Dead(index);
  variable_walk_8(rate,8,spw_floater);
  if(++clk2>84)
  {
    clk2=0;
    if(loopcnt)
      --loopcnt;
    else
    {
      /*if(id==ePATRA1||id==ePATRAL2||id==ePATRAL3)
      {
        if((misc%6)==0)
          loopcnt=3;
      }
      else
      {*/
        if((misc%dmisc6)==0)
          loopcnt=dmisc7;
      //}
    }
    ++misc;
  }
  double size=1;;
  for(int i=index+1; i<index+flycnt+1; i++)
  {                                                         //outside ring
    if (!adjusted)
    {
      if (get_bit(quest_rules,qr_NEWENEMYTILES))
      {
        /*switch (id)
        {
          case ePATRA1:
          ((enemy*)guys.spr(i))->o_tile=o_tile+40;
          break;
          case ePATRA2:
          ((enemy*)guys.spr(i))->o_tile=o_tile+40;
          break;
          case ePATRAL2:
          ((enemy*)guys.spr(i))->o_tile=o_tile+160;
          break;
          case ePATRAL3:
          ((enemy*)guys.spr(i))->o_tile=o_tile+160;
          break;
        }*/
        ((enemy*)guys.spr(i))->o_tile=o_tile+dmisc8;
      }
      else
      {
        ((enemy*)guys.spr(i))->o_tile=o_tile+1;
      }
	  ((enemy*)guys.spr(i))->cs=dmisc9;
      //        ((enemy*)guys.spr(i))->o_tile=192;
      /*if(id==ePATRAL2||id==ePATRAL3)
      {
        ((enemy*)guys.spr(i))->hp=9*DAMAGE_MULTIPLIER;
      }
      else
      {
        ((enemy*)guys.spr(i))->hp=6*DAMAGE_MULTIPLIER;
      }*/
      ((enemy*)guys.spr(i))->hp=dmisc3;
    }
    if(((enemy*)guys.spr(i))->hp <= 0)
    {
      for(int j=i; j<index+flycnt+flycnt2; j++)
      {
        guys.swap(j,j+1);
      }
      --flycnt;
    }
    else
    {
      int pos2 = ((enemy*)guys.spr(i))->misc;
      double a2 = (clk2-pos2*10.5)*PI/42;

      if(!dmisc4)
      {
        //maybe playing_field_offset here?
        if(loopcnt>0)
        {
          guys.spr(i)->x =  cos(a2+PI/2)*56*size - sin(pos2*PI/4)*28*size;
          guys.spr(i)->y = -sin(a2+PI/2)*56*size + cos(pos2*PI/4)*28*size;
        }
        else
        {
          guys.spr(i)->x =  cos(a2+PI/2)*28*size;
          guys.spr(i)->y = -sin(a2+PI/2)*28*size;
        }
        temp_x=guys.spr(i)->x;
        temp_y=guys.spr(i)->y;
      }
      else
      {
        circle_x =  cos(a2+PI/2)*42;
        circle_y = -sin(a2+PI/2)*42;

        if(loopcnt>0)
        {
          guys.spr(i)->x =  cos(a2+PI/2)*42;
          guys.spr(i)->y = (-sin(a2+PI/2)-cos(pos2*PI/4))*21;
        }
        else
        {
          guys.spr(i)->x = circle_x;
          guys.spr(i)->y = circle_y;
        }
        temp_x=circle_x;
        temp_y=circle_y;
      }
      double ddir=atan2(double(temp_y),double(temp_x));
      if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
      {
        guys.spr(i)->dir=l_down;
      }
      else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
      {
        guys.spr(i)->dir=left;
      }
      else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
      {
        guys.spr(i)->dir=l_up;
      }
      else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
      {
        guys.spr(i)->dir=up;
      }
      else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
      {
        guys.spr(i)->dir=r_up;
      }
      else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
      {
        guys.spr(i)->dir=right;
      }
      else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
      {
        guys.spr(i)->dir=r_down;
      }
      else
      {
        guys.spr(i)->dir=down;
      }
      guys.spr(i)->x += x;
      guys.spr(i)->y += y;
    }
  }

  size=.5;

  if (dmisc5)
  {
    if (dmisc5==1)
    {
      if(!(rand()&127))
      {
        addEwpn(x,y,z,wpn,1,wdp,dir,getUID());
      }
    }
    for(int i=index+flycnt+1; i<index+flycnt+flycnt2+1; i++)//inner ring
    {
      if (!adjusted)
      {
        ((enemy*)guys.spr(i))->hp=12*DAMAGE_MULTIPLIER;
        if (get_bit(quest_rules,qr_NEWENEMYTILES))
        {
          switch (dmisc5)
          {
            case 1:
            ((enemy*)guys.spr(i))->o_tile=o_tile+120;
            break;
            case 2:
            ((enemy*)guys.spr(i))->o_tile=o_tile+40;
            break;
          }
        }
        else
        {
          ((enemy*)guys.spr(i))->o_tile=o_tile+1;
        }
      }
      if (flycnt>0)
      {
        ((enemy*)guys.spr(i))->superman=true;
      }
      else
      {
        ((enemy*)guys.spr(i))->superman=false;
      }
      if(((enemy*)guys.spr(i))->hp <= 0)
      {
        for(int j=i; j<index+flycnt+flycnt2; j++)
        {
          guys.swap(j,j+1);
        }
        --flycnt2;
      }
      else
      {
        if (dmisc5==2)
        {
          if(!(rand()&127))
          {
            addEwpn(guys.spr(i)->x,guys.spr(i)->y,guys.spr(i)->z,wpn,1,wdp,dir,getUID());
          }
        }
        int pos2 = ((enemy*)guys.spr(i))->misc;
        double a2 = ((clk2-pos2*10.5)*PI/(42));

        if(loopcnt>0)
        {
          guys.spr(i)->x =  cos(a2+PI/2)*56*size - sin(pos2*PI/4)*28*size;
          guys.spr(i)->y = -sin(a2+PI/2)*56*size + cos(pos2*PI/4)*28*size;
        }
        else
        {
          guys.spr(i)->x =  cos(a2+PI/2)*28*size;
          guys.spr(i)->y = -sin(a2+PI/2)*28*size;
        }

        temp_x=guys.spr(i)->x;
        temp_y=guys.spr(i)->y;

        double ddir=atan2(double(temp_y),double(temp_x));
        if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
        {
          guys.spr(i)->dir=l_down;
        }
        else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
        {
          guys.spr(i)->dir=left;
        }
        else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
        {
          guys.spr(i)->dir=l_up;
        }
        else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
        {
          guys.spr(i)->dir=up;
        }
        else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
        {
          guys.spr(i)->dir=r_up;
        }
        else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
        {
          guys.spr(i)->dir=right;
        }
        else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
        {
          guys.spr(i)->dir=r_down;
        }
        else
        {
          guys.spr(i)->dir=down;
        }
        guys.spr(i)->x += x;
        guys.spr(i)->y = y-guys.spr(i)->y;

      }
    }
  }
  adjusted=true;
  return enemy::animate(index);
}

void ePatra::draw(BITMAP *dest)
{
  tile=o_tile;
  update_enemy_frame();
  enemy::draw(dest);
}

int ePatra::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
    case wWand:
    case wBeam:
    case wRefBeam:
    case wHammer:
    case wSword:
    if(flycnt||flycnt2)
      return 0;
    hp-=power;
    hclk=33;
    sfx(WAV_EHIT,pan(int(x)));
    sfx(WAV_GASP,pan(int(x)));
    return 1;
  }
  return 0;
}

void ePatra::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

esPatra::esPatra(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //cs=8;
  item_set=0;
  misc=clk;
  clk = -((misc*21)>>1)-1;
  yofs=playing_field_offset;
  hyofs=2;
  hxsz=hysz=12;
  hxofs=1000;
  mainguy=count_enemy=false;
  //o_tile=0;
}

bool esPatra::animate(int index)
{
  if(dying)
    return Dead();
  hxofs=4;
  return enemy::animate(index);
}

int esPatra::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
    case wWand:
    case wBeam:
    case wRefBeam:
    case wHammer:
    case wSword:
    hp-=power;
    hclk=33;
    sfx(WAV_EHIT,pan(int(x)));
    return 1;
  }
  return 0;
}

void esPatra::draw(BITMAP *dest)
{
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    tile = o_tile+(clk&3);
    switch (dir)                                            //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
      flip=0;
      break;
      case down:                                            //d
      flip=0; tile+=4;
      break;
      case left:                                            //l
      flip=0; tile+=8;
      break;
      case right:                                           //r
      flip=0; tile+=12;
      break;
      case l_up:                                            //ul
      flip=0; tile+=20;
      break;
      case r_up:                                            //ur
      flip=0; tile+=24;
      break;
      case l_down:                                          //dl
      flip=0; tile+=28;
      break;
      case r_down:                                          //dr
      flip=0; tile+=32;
      break;
    }
  }
  else
  {
    tile = o_tile+((clk&2)>>1);
  }
  if(clk>=0)
    enemy::draw(dest);
}

ePatraBS::ePatraBS(fix X,fix Y,int Id,int Clk) : enemy((fix)128,(fix)48,Id,Clk)
{
  //these are here to bypass compiler warnings about unused arguments
  X=X;
  Y=Y;

  adjusted=false;
  dir=(rand()&7)+8;
  step=0.25;
  //flycnt=6; flycnt2=0;
  flycnt=dmisc1;
  flycnt2=dmisc2;
  loopcnt=0;
  hxsz = 32;
  //nets+4480;
}

bool ePatraBS::animate(int index)
{
  if(dying)
    return Dead(index);
  variable_walk_8(rate,8,spw_floater);
  if(++clk2>90)
  {
    clk2=0;

    if(loopcnt)
      --loopcnt;
    else
    {
      if((misc%dmisc6)==0)
        loopcnt=dmisc7;
    }
    ++misc;
  }
  //    double size=1;;
  for(int i=index+1; i<index+flycnt+1; i++)
  {
    if (!adjusted)
    {
      ((enemy*)guys.spr(i))->hp=dmisc3;
      if (get_bit(quest_rules,qr_NEWENEMYTILES))
      {
        ((enemy*)guys.spr(i))->o_tile=o_tile+dmisc8;
      }
      else
      {
        ((enemy*)guys.spr(i))->o_tile=o_tile+1;
      }
	  ((enemy*)guys.spr(i))->cs = dmisc9;
    }
    if(((enemy*)guys.spr(i))->hp <= 0)
    {
      for(int j=i; j<index+flycnt+flycnt2; j++)
      {
        guys.swap(j,j+1);
      }
      --flycnt;
    }
    else
    {
      int pos2 = ((enemy*)guys.spr(i))->misc;
      double a2 = (clk2-pos2*15)*PI/45;
      temp_x =  cos(a2+PI/2)*45;
      temp_y = -sin(a2+PI/2)*45;

      if(loopcnt>0)
      {
        guys.spr(i)->x =  cos(a2+PI/2)*45;
        guys.spr(i)->y = (-sin(a2+PI/2)-cos(pos2*PI/3))*22.5;
      }
      else
      {
        guys.spr(i)->x = temp_x;
        guys.spr(i)->y = temp_y;
      }
      double ddir=atan2(double(temp_y),double(temp_x));
      if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
      {
        guys.spr(i)->dir=l_down;
      }
      else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
      {
        guys.spr(i)->dir=left;
      }
      else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
      {
        guys.spr(i)->dir=l_up;
      }
      else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
      {
        guys.spr(i)->dir=up;
      }
      else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
      {
        guys.spr(i)->dir=r_up;
      }
      else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
      {
        guys.spr(i)->dir=right;
      }
      else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
      {
        guys.spr(i)->dir=r_down;
      }
      else
      {
        guys.spr(i)->dir=down;
      }
      guys.spr(i)->x += x;
      guys.spr(i)->y += y;
    }
  }

  adjusted=true;
  return enemy::animate(index);
}

void ePatraBS::draw(BITMAP *dest)
{
  tile=o_tile;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    double ddir=atan2(double(y-(Link.y)),double(Link.x-x));
    if ((ddir<=(((-5)*PI)/8))&&(ddir>(((-7)*PI)/8)))
    {
      lookat=l_down;
    }
    else if ((ddir<=(((-3)*PI)/8))&&(ddir>(((-5)*PI)/8)))
      {
        lookat=down;
      }
      else if ((ddir<=(((-1)*PI)/8))&&(ddir>(((-3)*PI)/8)))
        {
          lookat=r_down;
        }
        else if ((ddir<=(((1)*PI)/8))&&(ddir>(((-1)*PI)/8)))
          {
            lookat=right;
          }
          else if ((ddir<=(((3)*PI)/8))&&(ddir>(((1)*PI)/8)))
            {
              lookat=r_up;
            }
            else if ((ddir<=(((5)*PI)/8))&&(ddir>(((3)*PI)/8)))
              {
                lookat=up;
              }
              else if ((ddir<=(((7)*PI)/8))&&(ddir>(((5)*PI)/8)))
                {
                  lookat=l_up;
                }
                else
                {
                  lookat=left;
                }
    switch (lookat)                                         //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
      flip=0;
      break;
      case down:                                            //d
      flip=0; tile+=8;
      break;
      case left:                                            //l
      flip=0; tile+=40;
      break;
      case right:                                           //r
      flip=0; tile+=48;
      break;
      case l_up:                                            //ul
      flip=0; tile+=80;
      break;
      case r_up:                                            //ur
      flip=0; tile+=88;
      break;
      case l_down:                                          //dl
      flip=0; tile+=120;
      break;
      case r_down:                                          //dr
      flip=0; tile+=128;
      break;
    }
    tile+=(2*(clk&3));
    xofs-=8;  yofs-=8;  drawblock(dest,15);
    xofs+=8;  yofs+=8;
  }
  else
  {
    flip=(clk&1);
    xofs-=8;  yofs-=8;  enemy::draw(dest);
    xofs+=16;           enemy::draw(dest);
    yofs+=16; enemy::draw(dest);
    xofs-=16;           enemy::draw(dest);
    xofs+=8;  yofs-=8;
  }
}

int ePatraBS::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
    case wWand:
    case wBeam:
    case wRefBeam:
    case wHammer:
    case wSword:
    if(flycnt||flycnt2)
      return 0;
    hp-=power;
    hclk=33;
    sfx(WAV_EHIT,pan(int(x)));
    sfx(WAV_GASP,pan(int(x)));
    return 1;
  }
  return 0;
}

void ePatraBS::death_sfx()
{
  sfx(WAV_GASP,pan(int(x)));
}

esPatraBS::esPatraBS(fix X,fix Y,int Id,int Clk) : enemy(X,Y,Id,Clk)
{
  //cs=csBOSS;
  item_set=0;
  misc=clk;
  clk = -((misc*21)>>1)-1;
  yofs=playing_field_offset;
  hyofs=2;
  hxsz=hysz=16;
  hxofs=1000;
  mainguy=count_enemy=false;
}

bool esPatraBS::animate(int index)
{
  if(dying)
    return Dead();
  hxofs=4;
  return enemy::animate(index);
}

int esPatraBS::takehit(int wpnId,int power,int wpnx,int wpny,int wpnDir)
{
  //these are here to bypass compiler warnings about unused arguments
  wpnx=wpnx;
  wpny=wpny;
  wpnDir=wpnDir;

  if(clk<0 || hclk>0 || superman)
    return 0;
  switch(wpnId)
  {
    case wPhantom:
    return 0;
    case wArrow:
    case wMagic:
    case wRefMagic:
    case wHookshot:
    case wBrang:
    sfx(WAV_CHINK,pan(int(x)));
    return 1;
    case wWand:
    case wBeam:
    case wRefBeam:
    case wHammer:
    case wSword:
    hp-=power;
    hclk=33;
    sfx(WAV_EHIT,pan(int(x)));
    return 1;
  }
  return 0;
}

void esPatraBS::draw(BITMAP *dest)
{
  tile=o_tile;
  if (get_bit(quest_rules,qr_NEWENEMYTILES))
  {
    switch (dir)                                            //directions get screwed up after 8.  *shrug*
    {
      case up:                                              //u
      flip=0;
      break;
      case down:                                            //d
      flip=0; tile+=4;
      break;
      case left:                                            //l
      flip=0; tile+=8;
      break;
      case right:                                           //r
      flip=0; tile+=12;
      break;
      case l_up:                                            //ul
      flip=0; tile+=20;
      break;
      case r_up:                                            //ur
      flip=0; tile+=24;
      break;
      case l_down:                                          //dl
      flip=0; tile+=28;
      break;
      case r_down:                                          //dr
      flip=0; tile+=32;
      break;
    }
    tile += ((clk&6)>>1);
  }
  else
  {
    tile += (clk&4)?1:0;
  }
  if(clk>=0)
    enemy::draw(dest);
}

/**********************************/
/**********  Misc Code  ***********/
/**********************************/

void addEwpn(int x,int y,int z,int id,int type,int power,int dir, int parentid)
{
  Ewpns.add(new weapon((fix)x,(fix)y,(fix)z,id,type,power,dir, parentid));
}

int enemy_dp(int index)
{
  return (((enemy*)guys.spr(index))->dp)<<2;
}

int ewpn_dp(int index)
{
  return (((weapon*)Ewpns.spr(index))->power)<<2;
}

int lwpn_dp(int index)
{
  return (((weapon*)Lwpns.spr(index))->power)<<2;
}

int hit_enemy(int index,int wpnId,int power,int wpnx,int wpny,int dir)
{
  return ((enemy*)guys.spr(index))->takehit(wpnId,power,wpnx,wpny,dir);
}

void enemy_scored(int index)
{
  ((enemy*)guys.spr(index))->scored=true;
}

void addguy(int x,int y,int id,int clk,bool mainguy)
{
  guy *g = new guy((fix)x,(fix)(y+(isdungeon()?1:0)),id,get_bit(quest_rules,qr_NOGUYPOOF)?0:clk,mainguy);
  guys.add(g);
}

void additem(int x,int y,int id,int pickup)
{
  item *i = new item((fix)x,(fix)y,(fix)0,id,pickup,0);
  items.add(i);
}

void additem(int x,int y,int id,int pickup,int clk)
{
  item *i = new item((fix)x,(fix)y,(fix)0,id,pickup,clk);
  items.add(i);
}

void kill_em_all()
{
  for(int i=0; i<guys.Count(); i++)
    ((enemy*)guys.spr(i))->kickbucket();
}

// For Link's hit detection. Don't count them if they are stunned or are guys.
int GuyHit(int tx,int ty,int tz,int txsz,int tysz,int tzsz)
{
  for(int i=0; i<guys.Count(); i++)
  {
    if(guys.spr(i)->hit(tx,ty,tz,txsz,tysz,tzsz))
    {
      if( ((enemy*)guys.spr(i))->stunclk==0 && (!get_bit(quest_rules, qr_SAFEENEMYFADE) || ((enemy*)guys.spr(i))->fading != fade_flicker)
		&&( ((enemy*)guys.spr(i))->d->family != eeGUY || ((enemy*)guys.spr(i))->dmisc1 ) )
      {
        return i;
      }
    }
  }
  return -1;
}

// For Link's hit detection. Count them if they are dying.
int GuyHit(int index,int tx,int ty,int tz,int txsz,int tysz,int tzsz)
{
  enemy *e = (enemy*)guys.spr(index);
  if(e->hp > 0)
    return -1;

  bool d = e->dying;
  int hc = e->hclk;
  e->dying = false;
  e->hclk = 0;
  bool hit = e->hit(tx,ty,tz,txsz,tysz,tzsz);
  e->dying = d;
  e->hclk = hc;

  return hit ? index : -1;
}

bool hasMainGuy()
{
  for(int i=0; i<guys.Count(); i++)
  {
    if(((enemy*)guys.spr(i))->mainguy)
    {
      return true;
    }
  }
  return false;
}

void EatLink(int index)
{
  ((eLikeLike*)guys.spr(index))->eatlink();
}

void GrabLink(int index)
{
  ((eWallM*)guys.spr(index))->grablink();
}

bool CarryLink()
{
  for(int i=0; i<guys.Count(); i++)
  {
    if(((guy*)(guys.spr(i)))->family==eeWALLM)
    {
      if(((eWallM*)guys.spr(i))->haslink)
      {
        Link.x=guys.spr(i)->x;
        Link.y=guys.spr(i)->y;
        return ((eWallM*)guys.spr(i))->misc > 0;
      }
    }
  }
  return false;
}

void movefairy(fix &x,fix &y,int misc)
{
  int i = guys.idFirst(eITEMFAIRY+0x1000*misc);
  if(i!=-1)
  {
    x = guys.spr(i)->x;
    y = guys.spr(i)->y;
  }
}

void killfairy(int misc)
{
  int i = guys.idFirst(eITEMFAIRY+0x1000*misc);
  guys.del(i);
}

bool addenemy(int x,int y,int id,int clk)
{
  return addenemy(x,y,0,id,clk);
}

bool addenemy(int x,int y,int z,int id,int clk)
{
  if(!(id&0xFFF)) return false;
  sprite *e=NULL;
  switch(guysbuf[id&0xFFF].family)
  {
    case eeSHOOT:
		{
			switch(guysbuf[id&0xFFF].misc10)
			{
			case 0:
				e = new eOctorok((fix)x,(fix)y,id,clk); break;
			case 1:
				e = new eMoblin((fix)x,(fix)y,id,clk); break;
			case 2:
				e = new eLynel((fix)x,(fix)y,id,clk); break;
			case 3:
				e = new eStalfos((fix)x,(fix)y,id,clk); break;
			case 4:
				e = new eDarknut((fix)x,(fix)y,id,clk); break;
			}
			break;
		}
	case eeWALK:
		{
			switch(guysbuf[id&0xFFF].misc10)
			{
			case 0:
				e = new eStalfos((fix)x,(fix)y,id,clk); break;
			case 1:
				e = new eDarknut((fix)x,(fix)y,id,clk); break;
			case 2:
				e = new eGibdo((fix)x,(fix)y,id,clk); break;
			}
			break;
		}
    case eeLEV:			  e = new eLeever((fix)x,(fix)y,id,clk); break;
    case eeTEK:			  e = new eTektite((fix)x,(fix)y,id,clk); break;
    case eePEAHAT:		  e = new ePeahat((fix)x,(fix)y,id,clk); break;
    case eeZORA:          e = new eZora((fix)x,(fix)y,id,clk); break;
    case eeROCK:
		{
			switch(guysbuf[id&0xFFF].misc10)
			{
			case 0:
				e = new eRock((fix)x,(fix)y,id,clk); break;
			case 1:
				e = new eBoulder((fix)x,(fix)y,id,clk); break;
			}
			break;
		}
    case eeARMOS:         e = new eArmos((fix)x,(fix)y,id,clk); break;
    case eeGHINI:		  e = new eGhini((fix)x,(fix)y,id,clk); break;
    case eeKEESE:		  e = new eKeese((fix)x,(fix)y,id,clk); break;
    case eeTRAP:
		{
			switch(guysbuf[id&0xFFF].misc10)
			{
			case 0:
				e = new eTrap((fix)x,(fix)y,id,clk); break;
			case 1:
				e = new eTrap2((fix)x,(fix)y,id,clk); break;
			}
			break;
		}
    case eeGEL:			  e = new eGel((fix)x,(fix)y,id,clk); break;
    case eeZOL:			  e = new eZol((fix)x,(fix)y,id,clk); break;
    case eeROPE:		  e = new eRope((fix)x,(fix)y,id,clk); break;
    case eeVIRE:          e = new eVire((fix)x,(fix)y,id,clk); break;
    case eeBUBBLE:        e = new eBubble((fix)x,(fix)y,id,clk); break;
    case eeLIKE:           e = new eLikeLike((fix)x,(fix)y,id,clk); break;
    case eePOLSV:         e = new ePolsVoice((fix)x,(fix)y,id,clk); break;
    case eeGORIYA:        e = new eGoriya((fix)x,(fix)y,id,clk); break;
    case eeWIZZ:          e = new eWizzrobe((fix)x,(fix)y,id,clk); break;
    case eePROJECTILE:    e = new eProjectile((fix)x,(fix)y,id,clk); break;
    case eeWALLM:          e = new eWallM((fix)x,(fix)y,id,clk); break;
    case eeDONGO:
		{
			switch(guysbuf[id&0xFFF].misc10)
			{
			case 0:
				e = new eDodongo((fix)x,(fix)y,id,clk); break;
			case 1:
				e = new eDodongo2((fix)x,(fix)y,id,clk); break;
			}
			break;
		}
    case eeAQUA:          e = new eAquamentus((fix)x,(fix)y,id,clk); break;
    case eeMOLD:          e = new eMoldorm((fix)x,(fix)y,id,guysbuf[id].misc1); break;
    case eeMANHAN:        e = new eManhandla((fix)x,(fix)y,id,clk); break;
    case eeGLEEOK:		  e = new eGleeok((fix)x,(fix)y,id,guysbuf[id].misc1); break;
    case eeDIG:
		{
			switch(guysbuf[id&0xFFF].misc10)
			{
			case 0:
				e = new eBigDig((fix)x,(fix)y,id,clk); break;
			case 1:
				e = new eLilDig((fix)x,(fix)y,id,clk); break;
			}
			break;
		}
    case eeGHOMA:			e = new eGohma((fix)x,(fix)y,id,clk); break;
    case eeLANM:			  e = new eLanmola((fix)x,(fix)y,id,guysbuf[id].misc1); break;
    case eePATRA:
		{
			switch(guysbuf[id&0xFFF].misc10)
			{
			case 0:
				e = new ePatra((fix)x,(fix)y,id,clk); break;
			case 1:
				e = new ePatraBS((fix)x,(fix)y,id,clk); break;
			}
			break;
		}
    case eeGANON:		  e = new eGanon((fix)x,(fix)y,id,clk); break;
    case eeKEESETRIB:     e = new eKeeseTrib((fix)x,(fix)y,id,clk); break;
    case eeVIRETRIB:      e = new eVireTrib((fix)x,(fix)y,id,clk); break;
    case eeGELTRIB:       e = new eGelTrib((fix)x,(fix)y,id,clk); break;
    case eeZOLTRIB:       e = new eZolTrib((fix)x,(fix)y,id,clk); break;
    case eeFAIRY:		  e = new eItemFairy((fix)x,(fix)y,id+0x1000*clk,clk); break;

	case eeFIRE:		  e = new eFire((fix)x,(fix)y,id,clk); break;
	case eeGUY:
		{
			switch(guysbuf[id&0xFFF].misc10)
			{
			case 0:
				e = new eNPC((fix)x,(fix)y,id,clk); break;
			case 1:
				e = new eTrigger((fix)x,(fix)y,id,clk); break;
			}
			break;
		}
    case eeSPINTILE:      e = new eSpinTile((fix)x,(fix)y,id,clk); break;

    case eeNONE:
		return true;
    default:              e = new enemy((fix)x,(fix)y,-1,clk); break;
  }
  if (z && canfall(id))
    e->z = (fix)z;
  if(!guys.add(e))
	  return false;
  if (z && canfall(id))
    ((enemy*)guys.spr(guys.Count()-1))->stunclk=(int)(z/GRAVITY); //Not exactly correct...

  // add segments of segmented enemies
  int c=0;
  switch(guysbuf[id].family)
  {

    case eeMOLD:
    for(int i=0; i<guysbuf[id].misc1; i++)
	{
		//christ this is messy -DD
	  int segclk = -i*((int)(8.0/(fix(guysbuf[id].step/100.0))));
	  if(!guys.add(new esMoldorm((fix)x,(fix)y,id+0x1000,segclk)))
	  {
		  for(int j=0; j<i+1; j++)
			  guys.del(guys.Count()-1);
		  return false;
	  }
	}
    break;

    //case eCENT1:

    //case eCENT2:
	case eeLANM:
    {
      int shft = guysbuf[id].misc2;
      if(!guys.add(new esLanmola((fix)x,(fix)y,id+0x1000,0)))
	  {
		  guys.del(guys.Count()-1);
		  return false;
	  }
      for(int i=1; i<guysbuf[id].misc1; i++)
	  {
        if(!guys.add(new esLanmola((fix)x,(fix)y,id+0x2000,-(i<<shft))))
		{
			for(int j=0; j<i; j++)
				guys.del(guys.Count()-1);
			return false;
		}
	  }
    } break;

    case eeMANHAN:
    for(int i=0; i<((!(guysbuf[id].misc2))?4:8); i++)
    {
      if(!guys.add(new esManhandla((fix)x,(fix)y,id+0x1000,i)))
	  {
		  for(int j=0; j<i+1; j++)
		  {
			  guys.del(guys.Count()-1);
		  }
		  return false;
	  }
      ((enemy*)guys.spr(guys.Count()-1))->frate=guysbuf[id].misc1;
    }
    break;

    case eeGLEEOK:
	{
		for(int i=0;i<guysbuf[id].misc1;i++)
		{
			if(!guys.add(new esGleeok((fix)x,(fix)y,id+0x1000,c, e)))
			{
				for(int j=0; j<i+1; j++)
				{
					guys.del(guys.Count()-1);
				}
				return false;
			}
			c-=guysbuf[id].misc4;
		}
	}
    break;


    case eePATRA:
	{
		switch(guysbuf[id].misc10)
		{
		case 0:
			for(int i=0;i<guysbuf[id].misc2;i++)
			{
				if(!guys.add(new esPatra((fix)x,(fix)y,id+0x1000,i)))
				{
					for(int j=0; j<i+1; j++)
						guys.del(guys.Count()-1);
					return false;
				}
			}
			for(int i=0;i<guysbuf[id].misc1;i++)
			{
				if(!guys.add(new esPatra((fix)x,(fix)y,id+0x1000,i)))
				{
					for(int j=0; j<i+1+guysbuf[id].misc2; j++)
						guys.del(guys.Count()-1);
					return false;
				}
			}
			break;
		case 1:
			for(int i=0;i<guysbuf[id].misc1;i++)
			{
				if(!guys.add(new esPatraBS((fix)x,(fix)y,id+0x1000,i)))
				{
					for(int j=0; j<i+1;j++)
						guys.del(guys.Count()-1);
					return false;
				}
			}
			break;
		}
		break;
	}
  }
  return true;
}

bool checkpos(int id)
{
  switch(guysbuf[id].family)
  {
    //case eTEK1:
    //case eTEK2:
    //case eTEK3: return false;
    case eeTEK: return false;
  }
  return true;
}

bool isjumper(int id)
{
  switch(guysbuf[id].family)
  {
    //case eTEK1:
    //case eTEK2:
    //case eTEK3: return true;
    case eeTEK: return true;
  }
  return false;
}

bool canfall(int id)
{
  switch (guysbuf[id&0xFFF].family)
  {
      case eeWALK:
	  case eeSHOOT:
	  case eeGEL:
	  case eeZOL:
	  case eeGELTRIB:
	  case eeZOLTRIB:
	  case eeARMOS:
	  case eeROPE:
	  case eeGORIYA:
	  case eeLIKE:
	  case eeDONGO:
	  case eeGHOMA:
      return true;
	  case eeGUY:
		  {
			  if(id < eOCTO1S)
				  return false;
			  switch(guysbuf[id].misc10)
			  {
			  case 1:
			  case 2:
				  return true;
			  case 0:
			  case 3:
				  return false;
			  }
		  }

  }
  return false;
}

void addfires()
{
  if (!get_bit(quest_rules,qr_NOGUYFIRES))
  {
    int bs = get_bit(quest_rules,qr_BSZELDA);
    addguy(bs? 64: 72,64,gFIRE,-17,false);
    addguy(bs?176:168,64,gFIRE,-18,false);
  }
}

void loadguys()
{
	if(loaded_guys)
		return;

	loaded_guys=true;

	byte Guy=0,Item=0;
	repaircharge=false;
	adjustmagic=false;
	learnslash=false;
	itemindex=-1;
	hasitem=0;

	if(currscr>=128 && DMaps[currdmap].flags&dmfGUYCAVES)
	{
		if (DMaps[currdmap].flags&dmfCAVES)
		{
			Guy=tmpscr[1].guy;
		}
	}
	else
	{
		Guy=tmpscr->guy;
		game->maps[(currmap<<7)+currscr] |= mVISITED;          // mark as visited
		if(Guy==gFAIRY)
		{
			sfx(WAV_SCALE);
			addguy(120,62,gFAIRY,-14,false);
			return;
		}
		if(DMaps[currdmap].flags&dmfGUYCAVES)
		{
			Guy=0;
		}
	}

	if(tmpscr->room==rZELDA)
	{
		addguy(120,72,Guy,-15,true);
		guys.spr(0)->hxofs=1000;
		addenemy(128,96,eFIRE,-15);
		addenemy(112,96,eFIRE,-15);
		addenemy(96,120,eFIRE,-15);
		addenemy(144,120,eFIRE,-15);
		return;
	}

	if(Guy)
	{
		addfires();
		if(currscr>=128)
			if(getmapflag())
				Guy=0;
		switch(tmpscr->room)
		{
		case rSP_ITEM:
		case rGRUMBLE:
		case rBOMBS:
		case rARROWS:
		case rSWINDLE:
		case rMUPGRADE:
		case rLEARNSLASH:
		case rTAKEONE:
			if(getmapflag())
				Guy=0;
			break;
		case rTRIFORCE:
			{
				int tc = TriforceCount();
				if(get_bit(quest_rules,qr_4TRI))
				{
					if((get_bit(quest_rules,qr_3TRI) && tc>=3) || tc>=4)
						Guy=0;
				}
				else
				{
					if((get_bit(quest_rules,qr_3TRI) && tc>=6) || tc>=8)
						Guy=0;
				}
			}
			break;
		}
		if(Guy)
		{
			blockpath=true;

			if(currscr<128)
				sfx(WAV_SCALE);
			addguy(120,64,Guy, (dlevel||BSZ)?-15:startguy[rand()&7], true);
			Link.Freeze();
		}
	}

	if(currscr<128)
	{
		Item=tmpscr->item;
		if(!getmapflag() && (tmpscr->hasitem != 0) && tmpscr->room!=rTAKEONE)
		{
			if(tmpscr->flags&fITEM)
				hasitem=1;
			else if(tmpscr->enemyflags&efCARRYITEM)
				hasitem=2;
			else
				items.add(new item((fix)tmpscr->itemx,
				(tmpscr->flags7&fITEMFALLS && tmpscr->flags7&fSIDEVIEW) ? (fix)-170 : (fix)tmpscr->itemy+1,
				(tmpscr->flags7&fITEMFALLS && !(tmpscr->flags7&fSIDEVIEW)) ? (fix)170 : (fix)0,
				Item,ipONETIME+ipBIGRANGE+((Item==iTriforce ||
				(tmpscr->flags3&fHOLDITEM)) ? ipHOLDUP : 0),0));
		}
	}
	else if(!(DMaps[currdmap].flags&dmfCAVES))
	{
		if(!getmapflag() && tmpscr[1].room==rSP_ITEM)
		{
			Item=tmpscr[1].catchall;
			if(Item)
				items.add(new item((fix)tmpscr->itemx,
				(tmpscr->flags7&fITEMFALLS && tmpscr->flags7&fSIDEVIEW) ? (fix)-170 : (fix)tmpscr->itemy+1,
				(tmpscr->flags7&fITEMFALLS && !(tmpscr->flags7&fSIDEVIEW)) ? (fix)170 : (fix)0,
				Item,ipONETIME|ipBIGRANGE|ipHOLDUP,0));
		}
	}

	if(tmpscr->room==r10RUPIES && !getmapflag())
	{
		setmapflag();
		for(int i=0; i<10; i++)
			additem(ten_rupies_x[i],ten_rupies_y[i],0,ipBIGRANGE,-14);
	}
}

void never_return(int index)
{
  if(!get_bit(quest_rules,qr_KILLALL))
    goto doit;

  for(int i=0; i<guys.Count(); i++)
    if(((((enemy*)guys.spr(i))->d->flags)&guy_neverret) && i!=index)
      goto dontdoit;

 doit:
  setmapflag(mNEVERRET);
dontdoit:
  return;
}

bool hasBoss()
{
  for(int i=0; i<guys.Count(); i++)
  {
    switch(guysbuf[guys.spr(i)->id].family)
    {
      case eeAQUA:
      case eeMOLD:
      case eeGHOMA:
      case eeGLEEOK:
      case eeMANHAN:
      case eeLANM:
      case eePATRA:
        return true;
        break;
      case eeDIG:
        switch(guysbuf[guys.spr(i)->id].misc10)
        {
          case 0:
            return true;
            break;
          case 1:
            return false;
            break;
        }
        break;
    }
  }
  return false;
}

bool slowguy(int id)
{
//  return (guysbuf[id].step<100);
//  return false; //only until I can find an easy way to resolve this problem. -jman
  switch(id)
  {
    case eOCTO1S:
    case eOCTO2S:
    case eOCTO1F:
    case eOCTO2F:
    case eLEV1:
    case eLEV2:
    case eROCK:
    case eBOULDER:
    return true;
  }
  return false;

}

bool countguy(int id)
{
  switch(guysbuf[id].family)
  {
    //case eSHOOTFBALL:
    //case eROCK:
    //case eBOULDER:
    //case eZORA:
    //case eTRAP:
	case eeROCK:
	case eeZORA:
	case eeTRAP:
    return false;
	case eePROJECTILE:
		if(guysbuf[id].weapon==ewFireball || guysbuf[id].weapon == ewFireball2)
			return false;
		break;
    //case eBUBBLEST:
    //case eBUBBLESP:
    //case eBUBBLESR:
    //case eBUBBLEIT:
    //case eBUBBLEIP:
    //case eBUBBLEIR:
	case eeBUBBLE:
    return dlevel != 0;
  }
  return true;
}

bool ok2add(int id)
{
  if (getmapflag(mNEVERRET) && (guysbuf[id].flags & guy_neverret)) return false;
  switch(guysbuf[id].family)
  {
    case eeGANON:
    case eeTRAP:
    return false;
	case eeDIG:
		{
			switch(guysbuf[id].misc10)
			{
			case 0:
				return true;
			case 1:
				if(!get_bit(quest_rules,qr_NOTMPNORET))
				  return !getmapflag(mTMPNORET);
				return true;
			}
		}
  }

  if(!get_bit(quest_rules,qr_NOTMPNORET))
    return !getmapflag(mTMPNORET);
  return true;
}

void load_default_enemies()
{
  wallm_load_clk=frame-80;
  int Id=0;

  if(tmpscr->enemyflags&efZORA)
  {
    for(int i=0;i<eMAXGUYS;i++)
	{
		if(guysbuf[i].flags2 & eneflag_zora)
		{
			Id = i;
			break;
		}
	}
    addenemy(-16,-16,Id,0);
  }

  if(tmpscr->enemyflags&efTRAP4)
  {
    for(int i=0;i<eMAXGUYS;i++)
	{
		if(guysbuf[i].flags2 & eneflag_trap)
		{
			Id = i;
			break;
		}
	}
    addenemy(32,32,Id,-14);
    addenemy(208,32,Id,-14);
    addenemy(32,128,Id,-14);
    addenemy(208,128,Id,-14);
  }

  for(int y=0; y<176; y+=16)
  {
    for(int x=0; x<256; x+=16)
    {
      int ctype = combobuf[MAPCOMBO(x,y)].type;
      int cflag = MAPFLAG(x, y);
      int cflag2 = MAPCOMBOFLAG(x, y);
      if((ctype==cTRAP_H)||(cflag==mfTRAP_H)||(cflag2==mfTRAP_H))
	  {
	    for(int i=0;i<eMAXGUYS;i++)
	    {
		    if(guysbuf[i].flags2 & cmbflag_trph)
		    {
			    Id = i;
			    break;
		    }
	    }
        addenemy(x,y,Id,-14);
	  }
      if((ctype==cTRAP_V)||(cflag==mfTRAP_V)||(cflag2==mfTRAP_V))
	  {
	    for(int i=0;i<eMAXGUYS;i++)
	    {
		    if(guysbuf[i].flags2 & cmbflag_trpv)
		    {
			    Id = i;
			    break;
		    }
	    }
        addenemy(x,y,Id,-14);
	  }
      if((ctype==cTRAP_4)||(cflag==mfTRAP_4)||(cflag2==mfTRAP_4))
      {
	    for(int i=0;i<eMAXGUYS;i++)
	    {
		    if(guysbuf[i].flags2 & cmbflag_trp4)
		    {
			    Id = i;
			    break;
		    }
	    }
        if(addenemy(x,y,Id,-14))
			guys.spr(guys.Count()-1)->dummy_int[1]=2;
      }
      if((ctype==cTRAP_LR)||(cflag==mfTRAP_LR)||(cflag2==mfTRAP_LR))
	  {
	    for(int i=0;i<eMAXGUYS;i++)
	    {
		    if(guysbuf[i].flags2 & cmbflag_trplr)
		    {
			    Id = i;
			    break;
		    }
	    }
        addenemy(x,y,eTRAP_LR,-14);
	  }
      if((ctype==cTRAP_UD)||(cflag==mfTRAP_UD)||(cflag2==mfTRAP_UD))
	  {
	    for(int i=0;i<eMAXGUYS;i++)
	    {
		    if(guysbuf[i].flags2 & cmbflag_trpud)
		    {
			    Id = i;
			    break;
		    }
	    }
        addenemy(x,y,eTRAP_UD,-14);
	  }

    }
  }
  if(tmpscr->enemyflags&efTRAP2)
  {
    for(int i=0;i<eMAXGUYS;i++)
	{
		if(guysbuf[i].flags2 & eneflag_trp2)
		{
			Id = i;
			break;
		}
	}
    if(addenemy(64,80,Id,-14))
		guys.spr(guys.Count()-1)->dummy_int[1]=1;
    if(addenemy(176,80,Id,-14))
		guys.spr(guys.Count()-1)->dummy_int[1]=1;
  }

  if(tmpscr->enemyflags&efROCKS)
  {
    for(int i=0;i<eMAXGUYS;i++)
	{
		if(guysbuf[i].flags2 & eneflag_rock)
		{
			Id = i;
			break;
		}
	}
    addenemy(rand()&0xF0,0,Id,0);
    addenemy(rand()&0xF0,0,Id,0);
    addenemy(rand()&0xF0,0,Id,0);
  }

  if(tmpscr->enemyflags&efFIREBALLS)
  {
    for(int y=0; y<176; y+=16)
	{
      for(int x=0; x<256; x+=16)
      {
	    for(int i=0;i<eMAXGUYS;i++)
	    {
		    if(guysbuf[i].flags2 & eneflag_fire)
		    {
			    Id = i;
			    break;
		    }
	    }
        int ctype = combobuf[MAPCOMBO(x,y)].type;
        if(ctype==cL_STATUE)
          addenemy(x+4,y+7,Id,24);
        if(ctype==cR_STATUE)
          addenemy(x-8,y-1,Id,24);
        if(ctype==cC_STATUE)
          addenemy(x,y,Id,24);
      }
	}
  }
}

void nsp(bool random)
  // moves sle_x and sle_y to the next position
{
  if (random) {
    if (rand()%2) {
      sle_x = (rand()%2) ? 0 : 240;
      sle_y = (rand()%10)*16;
    } else {
      sle_y = (rand()%2) ? 0 : 160;
      sle_x = (rand()%15)*16;
    }
    return;
  }

  if(sle_x==0)
  {
	  if(sle_y<160)
		  sle_y+=16;
	  else
		  sle_x+=16;
  }
  else if(sle_y==160)
  {
	  if(sle_x<240)
		  sle_x+=16;
	  else
		  sle_y-=16;
  }
  else if(sle_x==240)
  {
	  if(sle_y>0)
		  sle_y-=16;
	  else
		  sle_x-=16;
  }
  else if(sle_y==0)
  {
	  if(sle_x>0)
		  sle_x-=16;
	  else
		  sle_y+=16;
  }
}

int next_side_pos(bool random)
  // moves sle_x and sle_y to the next available position
  // returns the direction the enemy should face
{
  bool blocked;
  int c=0;
  do
  {
    nsp(random);
	blocked = _walkflag(sle_x,sle_y,2) || _walkflag(sle_x,sle_y+8,2) || (MAPFLAG(sle_x,sle_y) == mfNOENEMY || MAPCOMBOFLAG(sle_x,sle_y)==mfNOENEMY);
	if(++c>50)
      return -1;
  } while(blocked);

  int dir=0;
  if(sle_x==0)    dir=right;
  if(sle_y==0)    dir=down;
  if(sle_x==240)  dir=left;
  if(sle_y==168)  dir=up;
  return dir;
}

bool can_side_load(int id)
{
  switch (guysbuf[id].family)//id&0x0FFF)
  {
    //case eTEK1:
    //case eTEK2:
    //case eTEK3:
    //case eLEV1:
    //case eLEV2:
    //case eLEV3:
    //case eRAQUAM:
    //case eLAQUAM:
    //case eDODONGO:
    //case eMANHAN:
    //case eGLEEOK1:
    //case eGLEEOK2:
    //case eGLEEOK3:
    //case eGLEEOK4:
    //case eDIG1:
    //case eDIG3:
    //case eGOHMA1:
    //case eGOHMA2:
    //case eCENT1:
    //case eCENT2:
    //case ePATRA1:
    //case ePATRA2:
    //case eGANON:
    //case eMANHAN2:
    //case eCEILINGM: later
    //case eFLOORM: later
    //case ePATRABS:
    //case ePATRAL2:
    //case ePATRAL3:
    //case eGLEEOK1F:
    //case eGLEEOK2F:
    //case eGLEEOK3F:
    //case eGLEEOK4F:
    //case eDODONGOBS:
    //case eDODONGOF:
    //case eGOHMA3:
    //case eGOHMA4:
    //case eSHOOTMAGIC:
    //case eSHOOTROCK:
    //case eSHOOTSPEAR:
    //case eSHOOTSWORD:
    //case eSHOOTFLAME:
    //case eSHOOTFLAME2:
    //case eSHOOTFBALL:
	case eeTEK:
	case eeLEV:
	case eeAQUA:
	case eeDONGO:
	case eeMANHAN:
	case eeGLEEOK:
	case eeDIG:
	case eeGHOMA:
	case eeLANM:
	case eePATRA:
	case eeGANON:
	case eePROJECTILE:
    return false;
    break;
  }
  return true;
}


void side_load_enemies()
{
  if(sle_clk==0)
  {
    sle_cnt = 0;
    int guycnt = 0;
    short s = (currmap<<7)+currscr;
    bool beenhere=false;
    bool reload=true;
	bool reloadspecial = false;

    load_default_enemies();

    for(int i=0; i<6; i++)
      if(visited[i]==s)
        beenhere=true;

      if(!beenhere)
    {
      visited[vhead]=s;
      vhead = (vhead+1)%6;
    }
    else if(game->guys[s]==0)
      {
        sle_cnt=0;
        reload=false;
		reloadspecial = true;
      }

      if(reload)
    {
      sle_cnt = game->guys[s];
	  if(sle_cnt==0)
      {
        while(sle_cnt<10 && tmpscr->enemy[sle_cnt]!=0)
          ++sle_cnt;
      }
	  else
		  reloadspecial = true;
    }

    if((get_bit(quest_rules,qr_ALWAYSRET)) || (tmpscr->flags3&fENEMIESRETURN))
    {
      sle_cnt = 0;
	  reloadspecial = false;
      while(sle_cnt<10 && tmpscr->enemy[sle_cnt]!=0)
        ++sle_cnt;
    }

	if(reloadspecial)
	{
		for(int i=0; !countguy(tmpscr->enemy[i]) && sle_cnt<10; i++)
		  ++sle_cnt;
	}

    for(int i=0; i<sle_cnt; i++)
      if(countguy(tmpscr->enemy[i]))
        ++guycnt;

      game->guys[s] = guycnt;
  }

  if( (++sle_clk+8)%24 == 0)
  {
    int dir = next_side_pos(tmpscr->pattern==pSIDESR);
    if(dir==-1 || tooclose(sle_x,sle_y,32))
    {
      return;
    }
    int enemy_slot=guys.Count();
    if(addenemy(sle_x,sle_y,tmpscr->enemy[--sle_cnt],0))
		guys.spr(enemy_slot)->dir = dir;
  }

  if(sle_cnt<=0)
    loaded_enemies=true;
}

void loadenemies()
{
  if(loaded_enemies)
    return;

  if(tmpscr->pattern==pSIDES || tmpscr->pattern==pSIDESR)
  {
    side_load_enemies();
    return;
  }

  loaded_enemies=true;

  // do enemies that are always loaded
  load_default_enemies();

  // dungeon basements

  static byte dngn_enemy_x[4] = {32,96,144,208};

  if(currscr>=128)
  {
    if(DMaps[currdmap].flags&dmfCAVES) return;
    for(int i=0; i<4; i++)
      addenemy(dngn_enemy_x[i],96,tmpscr->enemy[i]?tmpscr->enemy[i]:(int)eKEESE1,-14-i);
    return;
  }

  // check if it's been long enough to reload all enemies

  int loadcnt = 10;
  short s = (currmap<<7)+currscr;
  bool beenhere = false;
  bool reload = true;

  for(int i=0; i<6; i++)
    if(visited[i]==s)
      beenhere = true;

    if(!beenhere)
  {
    visited[vhead]=s;
    vhead = (vhead+1)%6;
  }
  else if(game->guys[s]==0)
    {
      loadcnt = 0;
      reload  = false;
    }

    if(reload)
  {
    loadcnt = game->guys[s];
    if(loadcnt==0)
      loadcnt = 10;
  }

  if((get_bit(quest_rules,qr_ALWAYSRET)) || (tmpscr->flags3&fENEMIESRETURN))
    loadcnt = 10;

  for(int i=0; !countguy(tmpscr->enemy[i]) && loadcnt<10; i++)
    ++loadcnt;

  // check if it's the dungeon boss and it has been beaten before
  if(tmpscr->enemyflags&efBOSS && game->lvlitems[dlevel]&liBOSS)
    return;

  // load enemies

  if(tmpscr->pattern==pRANDOM || tmpscr->pattern==pCEILING)                    // enemies appear at random places
  {
    //int set=loadside*9;
    int pos=rand()%9;
    int clk=-15,x,y,fastguys=0;
    int i=0,guycnt=0;

    for( ; i<loadcnt && tmpscr->enemy[i]>0; i++)            /* i=0 */
    {
      bool placed=false;
      int t=0;
      for(int sy=0; sy<176; sy+=16)
      {
        for(int sx=0; sx<256; sx+=16)
        {
          int cflag = MAPFLAG(sx, sy);
          int cflag2 = MAPCOMBOFLAG(sx, sy);
          if(((cflag==mfENEMY0+i)||(cflag2==mfENEMY0+i)) && (!placed))
          {
            if(!ok2add(tmpscr->enemy[i]))
              ++loadcnt;
            else
            {
              addenemy(sx,
                (tmpscr->pattern==pCEILING && tmpscr->flags7&fSIDEVIEW) ? -(150+50*guycnt) : sy,
                (tmpscr->pattern==pCEILING && !(tmpscr->flags7&fSIDEVIEW)) ? 150+50*guycnt : 0,tmpscr->enemy[i],-15);
              if(countguy(tmpscr->enemy[i]))
                ++guycnt;

              placed=true;
              goto placed_enemy;
            }
          }
        }
      }
      do
      {
        if(pos>=9)
          pos-=9;
        x=stx[loadside][pos];
        y=sty[loadside][pos];
        ++pos;
        ++t;
      } while(
              t<20 &&
              (
               (
                (
                 isflier(tmpscr->enemy[i])&&
                 (
                  (
                   COMBOTYPE(x+8,y+8)==cNOFLYZONE||
				   COMBOTYPE(x+8,y+8)==cNOENEMY||
				   MAPFLAG(x+8,y+8)==mfNOENEMY||
				   MAPCOMBOFLAG(x+8,y+8)==mfNOENEMY
                  )||
                  (
                   _walkflag(x,y+8,2)&&
                   !get_bit(quest_rules,qr_WALLFLIERS)
                  )
                 )
                ) ||
                (
                 isjumper(tmpscr->enemy[i])&&
				 (
                  COMBOTYPE(x+8,y+8)==cNOJUMPZONE||
				  COMBOTYPE(x+8,y+8)==cNOENEMY||
				  MAPFLAG(x+8,y+8)==mfNOENEMY||
				  MAPCOMBOFLAG(x+8,y+8)==mfNOENEMY
				 )
                ) ||
                (
                 !isflier(tmpscr->enemy[i])&&
                 !isjumper(tmpscr->enemy[i])&&
                 (
                  _walkflag(x,y+8,2)||
                  COMBOTYPE(x+8,y+8)==cPIT||
                  COMBOTYPE(x+8,y+8)==cPITB||
                  COMBOTYPE(x+8,y+8)==cPITC||
                  COMBOTYPE(x+8,y+8)==cPITD||
                  COMBOTYPE(x+8,y+8)==cPITR||
				  COMBOTYPE(x+8,y+8)==cNOENEMY||
				  MAPFLAG(x+8,y+8)==mfNOENEMY||
				  MAPCOMBOFLAG(x+8,y+8)==mfNOENEMY

                 )
                )&&
                tmpscr->enemy[i]!=eZORA
               ) ||
               (
                tooclose(x,y,40) &&
                t<11
               )
              )
             );

      if(t < 20)
      {
        int c=0;
        c=clk;
        if(!slowguy(tmpscr->enemy[i]))
          ++fastguys;
        else if(fastguys>0)
            c=-15*(i-fastguys+2);
          else
            c=-15*(i+1);

        if(BSZ)
          c=-15;

        if(!ok2add(tmpscr->enemy[i]))
          ++loadcnt;
        else
        {
          addenemy(x,(tmpscr->pattern==pCEILING && tmpscr->flags7&fSIDEVIEW) ? -(150+50*guycnt) : y,
		     (tmpscr->pattern==pCEILING && !(tmpscr->flags7&fSIDEVIEW)) ? 150+50*guycnt : 0,tmpscr->enemy[i],c);
          if(countguy(tmpscr->enemy[i]))
            ++guycnt;
        }

        if(i==0 && tmpscr->enemyflags&efLEADER)
        {
          int index = guys.idFirst(tmpscr->enemy[i],0xFFF);
          if (index!=-1)
          {
            ((enemy*)guys.spr(index))->leader = true;
          }
        }
        if(i==0 && hasitem==2)
        {
          int index = guys.idFirst(tmpscr->enemy[i],0xFFF);
          if (index!=-1)
          {
            ((enemy*)guys.spr(index))->itemguy = true;
          }
        }
      }                                                     // if(t < 20)
   placed_enemy:
      if (placed)
      {
        if(i==0 && tmpscr->enemyflags&efLEADER)
        {
          int index = guys.idFirst(tmpscr->enemy[i],0xFFF);
          if (index!=-1)
          {
            ((enemy*)guys.spr(index))->leader = true;
          }
        }
        if(i==0 && hasitem==2)
        {
          int index = guys.idFirst(tmpscr->enemy[i],0xFFF);
          if (index!=-1)
          {

            ((enemy*)guys.spr(index))->itemguy = true;
          }
        }
      }
      --clk;
    }                                                       // for
    game->guys[s] = guycnt;
  }
}

void moneysign()
{
  additem(48,108,iRupy,ipDUMMY);
  //  textout(scrollbuf,zfont,"X",64,112,CSET(0)+1);
  set_clip_state(pricesdisplaybuf, 0);
  textout_ex(pricesdisplaybuf,zfont,"X",64,112,CSET(0)+1,-1);
}

void putprices(bool sign)
{
  // refresh what's under the prices
  // for(int i=5; i<12; i++)
  //   putcombo(scrollbuf,i<<4,112,tmpscr->data[112+i],tmpscr->cpage);

  rectfill(pricesdisplaybuf, 72, 112, pricesdisplaybuf->w-1, pricesdisplaybuf->h-1, 0);  int step=32;
  int x=80;
  if(prices[2][0]==0&&prices[2][1]==0)
  {
    step<<=1;
    if(prices[1][0]==0&&prices[1][1]==0)
    {
      x=112;
    }
  }
  for(int i=0; i<3; i++)
  {
    if(prices[i][0])
    {
      char buf[8];
      sprintf(buf,sign?"%+3d":"%3d",prices[i][0]);

      int l=(int)strlen(buf);
      set_clip_state(pricesdisplaybuf, 0);
      textout_ex(pricesdisplaybuf,zfont,buf,x-(l>3?(l-3)<<3:0),112,CSET(0)+1,-1);
    }
    x+=step;
  }
}

void setupscreen()
{
  boughtsomething=false;
  int t=currscr<128?0:1;
  word str=tmpscr[t].str;

  for(int i=0; i<3; i++)
  {
    prices[i][0]=0;
    prices[i][1]=0;
  }

  switch(tmpscr[t].room)
  {
    case rSP_ITEM:                                          // special item
    additem(120,89,tmpscr[t].catchall,ipONETIME+ipHOLDUP+ipCHECK);
    break;

    case rINFO:                                             // pay for info
    {
      int count = 0;
      int base  = 88;
      int step  = 5;

      moneysign();
      for(int i=0; i<3; i++)
      {
        if(QMisc.info[tmpscr[t].catchall].str[i])
        {
          ++count;
        }
        else
          break;
      }
      if(count)
      {
        if(count==1)
        {
          base = 88+32;
        }
        if(count==2)
        {
          step = 6;
        }

        for(int i=0; i < count; i++)
        {
          additem((i << step)+base, 89, iRupy, ipMONEY + ipDUMMY);
          prices[i][0] = -(QMisc.info[tmpscr[t].catchall].price[i]);

	  int itemid = current_item_id(itype_wealthmedal);
	  if(itemid >= 0)
	  {
		if (itemsbuf[itemid].flags & ITEM_MISC1_PER)
			prices[i][0]=((prices[i][0]*itemsbuf[itemid].misc1)/100);
		else
			prices[i][0]+=itemsbuf[itemid].misc1;
	  }
	  if ((QMisc.info[tmpscr[t].catchall].price[i])>1 && prices[i][0]>-1)
	    prices[i][0]=-1;
        }
      }
      break;
    }

    case rMONEY:                                            // secret money
    additem(120,89,iRupy,ipONETIME+ipDUMMY+ipMONEY);
    break;

    case rGAMBLE:                                           // gambling
    prices[0][0]=prices[1][0]=prices[2][0]=-10;
    moneysign();
    additem(88,89,iRupy,ipMONEY+ipDUMMY);
    additem(120,89,iRupy,ipMONEY+ipDUMMY);
    additem(152,89,iRupy,ipMONEY+ipDUMMY);
    break;

    case rREPAIR:                                           // door repair
                                                            //  if (!get_bit(quest_rules,qr_REPAIRFIX)) {
    setmapflag();
    //  }
    repaircharge=true;
    break;

    case rMUPGRADE:                                         // upgrade magic
    adjustmagic=true;
    break;

    case rLEARNSLASH:                                       // learn slash attack
    learnslash=true;
    break;

    case rRP_HC:                                            // heart container or red potion
    additem(88,89,iRPotion,ipONETIME+ipHOLDUP+ipFADE);
    additem(152,89,iHeartC,ipONETIME+ipHOLDUP+ipFADE);
    break;

    case rP_SHOP:                                           // potion shop
    if(current_item(itype_letter)<i_letter_used)
    {
      str=0;
      break;
    }
    // fall through

    case rTAKEONE:                                          // take one
    case rSHOP:                                             // shop
    {
      int count = 0;
      int base  = 88;
      int step  = 5;

      if (tmpscr[t].room != rTAKEONE)
        moneysign();
      //count and align the stuff
      for(int i=0; i<3; ++i)
      {
        if (QMisc.shop[tmpscr[t].catchall].hasitem[count] != 0)
        {
          ++count;
        }
        else
        {
          break;
        }
      }

      if(count==1)
      {
        base = 88+32;
      }
      if(count==2)
      {
        step = 6;
      }

      for(int i=0; i<count; i++)
      {
        additem((i<<step)+base, 89, QMisc.shop[tmpscr[t].catchall].item[i], ipHOLDUP+ipFADE+(tmpscr[t].room == rTAKEONE ? ipONETIME : ipCHECK));
        if (tmpscr[t].room != rTAKEONE)
        {
	  prices[i][0] = QMisc.shop[tmpscr[t].catchall].price[i];
	  int itemid = current_item_id(itype_wealthmedal);
	  if(itemid >= 0)
	  {
		if (itemsbuf[itemid].flags & ITEM_MISC1_PER)
			prices[i][0]=((prices[i][0]*itemsbuf[itemid].misc1)/100);
		else
			prices[i][0]+=itemsbuf[itemid].misc1;
	  }
	  if ((QMisc.shop[tmpscr[t].catchall].price[i])>1 && prices[i][0]<1)
	    prices[i][0]=1;
        }
      }
      break;
    }

    case rBOMBS:                                            // more bombs
    additem(120,89,iRupy,ipDUMMY+ipMONEY);
    prices[0][0]=-tmpscr[t].catchall;
    break;

    case rARROWS:                                            // more arrows
    additem(120,89,iRupy,ipDUMMY+ipMONEY);
    prices[0][0]=-tmpscr[t].catchall;
    break;

    case rSWINDLE:                                          // leave heart container or money
    additem(88,89,iHeartC,ipDUMMY+ipMONEY);
    additem(152,89,iRupy,ipDUMMY+ipMONEY);
    prices[0][0]=-1;
    prices[1][0]=-tmpscr[t].catchall;
    break;

  }
  if (tmpscr[t].room == rSWINDLE || tmpscr[t].room == rBOMBS || tmpscr[t].room == rARROWS)
  {
	int i = (tmpscr[t].room == rSWINDLE ? 1 : 0);
	  int itemid = current_item_id(itype_wealthmedal);
	  if(itemid >= 0)
	  {
		if (itemsbuf[itemid].flags & ITEM_MISC1_PER)
			prices[i][0]*=(itemsbuf[itemid].misc1/100);
		else
			prices[i][0]+=itemsbuf[itemid].misc1;
	  }
	if (tmpscr[t].catchall>1 && prices[i][0]>-1)
	  prices[i][0]=-1;
  }

  putprices(false);

  if(str)
  {
    msgstr=str;
    msgfont=setmsgfont();
    msgcset=0;
    msgcolour=1;
    msgspeed=5;
    msgclk=msgpos=msgptr=0;
    if (MsgStrings[msgstr].tile!=0)
    {
      frame2x2(msgdisplaybuf,&QMisc,24,MsgStrings[msgstr].y,MsgStrings[msgstr].tile,MsgStrings[msgstr].cset,
		   26,5,0,0,0);
    }
  }
  else

  {
    Link.unfreeze();
  }
}

FONT *setmsgfont()
{
    switch(MsgStrings[msgstr].font)
    {
	default:
	  return zfont;
	case font_z3font:
	  return z3font;
	case font_z3smallfont:
	  return z3smallfont;
	case font_deffont:
	  return deffont;
	case font_lfont:
	  return lfont;
	case font_lfont_l:
	  return lfont_l;
	case font_pfont:
	  return pfont;
	case font_mfont:
	  return mfont;
	case font_ztfont:
	  return ztfont;
	case font_sfont:
	  return sfont;
	case font_sfont2:
	  return sfont2;
	case font_spfont:
	  return spfont;
	case font_ssfont1:
	  return ssfont1;
	case font_ssfont2:
	  return ssfont2;
	case font_ssfont3:
	  return ssfont3;
	case font_ssfont4:
	  return ssfont4;
	case font_gbzfont:
	  return gbzfont;
	case font_goronfont:
	  return goronfont;
	case font_zoranfont:
	  return zoranfont;
	case font_hylian1font:
	  return hylian1font;
	case font_hylian2font:
	  return hylian2font;
	case font_hylian3font:
	  return hylian3font;
	case font_hylian4font:
	  return hylian4font;
    }
}

bool parsemsgcode()
{
  if (msgptr>=MSGSIZE-2) return false;
  switch(MsgStrings[msgstr].s[msgptr]-1)
  {
    case MSGC_COLOUR:
      msgcset = (MsgStrings[msgstr].s[++msgptr]-1);
      msgcolour = (MsgStrings[msgstr].s[++msgptr]-1);
      return true;
    case MSGC_SPEED:
      msgspeed=MsgStrings[msgstr].s[++msgptr]-1;
      return true;
    case MSGC_CTRUP:
      game->change_counter( MsgStrings[msgstr].s[msgptr+2]-1, MsgStrings[msgstr].s[msgptr+1]-1);
      msgptr+=2;
      return true;
    case MSGC_CTRDN:
      game->change_counter( MsgStrings[msgstr].s[msgptr+2]-1, -(MsgStrings[msgstr].s[msgptr+1]-1));
      msgptr+=2;
      return true;
    case MSGC_CTRSET:
      game->set_counter( MsgStrings[msgstr].s[msgptr+2]-1, MsgStrings[msgstr].s[msgptr+1]-1);
      msgptr+=2;
      return true;
    case MSGC_CTRUPPC:
    case MSGC_CTRDNPC:
    case MSGC_CTRSETPC:
    {
      int counter = MsgStrings[msgstr].s[++msgptr]-1;
      int amount = (int)(((MsgStrings[msgstr].s[++msgptr]-1)/100)*game->get_maxcounter( counter));
      if (MsgStrings[msgstr].s[msgptr]==MSGC_CTRDNPC)
	amount*=-1;
      if (MsgStrings[msgstr].s[msgptr]==MSGC_CTRSETPC)
        game->set_counter( amount, counter);
      else
        game->change_counter( amount, counter);
      return true;
    }
    case MSGC_SFX:
      sfx((int)MsgStrings[msgstr].s[++msgptr]-1,128);
      return true;
/*
    case MSGC_NAME:
      if (!((cBbtn()&&get_bit(quest_rules,qr_ALLOWMSGBYPASS)) || msgspeed==0))
        sfx(MsgStrings[msgstr].sfx);
      textprintf_ex(msgdisplaybuf,msgfont,((msgpos%24)<<3)+32,((msgpos/24)<<3)+(MsgStrings[msgstr].y)+8,CSET(msgcset)+msgcolour,-1,
                    "%s",game->get_name());
      return true;
*/
    case MSGC_GOTOIF:
      if (current_item(getItemFamily(itemsbuf,(int)MsgStrings[msgstr].s[++msgptr]-1))>=itemsbuf[(int)MsgStrings[msgstr].s[msgptr]-1].fam_type)
        goto switched;
      ++msgptr;
      return true;
    case MSGC_GOTOIFCTR:
      if (game->get_counter((int)MsgStrings[msgstr].s[++msgptr]-1)>=(int)MsgStrings[msgstr].s[++msgptr]-1)
	goto switched;
      ++msgptr;
      return true;
    case MSGC_GOTOIFCTRPC:
    {
      int counter = MsgStrings[msgstr].s[++msgptr]-1;
      int amount = (int)(((MsgStrings[msgstr].s[++msgptr]-1)/100)*game->get_maxcounter( counter));
      if (game->get_counter(counter)>=amount)
	goto switched;
      ++msgptr;
      return true;
    }
    case MSGC_GOTOIFTRICOUNT:
      if (TriforceCount() >= (int)(MsgStrings[msgstr].s[++msgptr]-1))
	goto switched;
      ++msgptr;
      return true;
    case MSGC_GOTOIFTRI:
      if(game->lvlitems[(int)(MsgStrings[msgstr].s[++msgptr]-1)]&liTRIFORCE)
	goto switched;
      ++msgptr;
      return true;
    case MSGC_GOTOIFRAND:
      if(!(rand()%(int)(MsgStrings[msgstr].s[++msgptr]-1)))
	goto switched;
      ++msgptr;
      return true;
#if 0
    /* It seems that global_d[] should be used by ZScripts only! */
    case MSGC_GOTOIFGLOBAL:
      if (game->global_d[(int)(MsgStrings[msgstr].s[++msgptr]-1)]>=(int)(MsgStrings[msgstr].s[++msgptr]-1))
	goto switched;
      ++msgptr;
      return true;
#endif
switched:
      linkedmsgclk=0;
      msgstr=MsgStrings[msgstr].s[++msgptr]-1;
      msgpos=msgptr=msgcset=0;
      msgcolour=1;
      msgspeed=5;
      msgfont=setmsgfont();
      clear_bitmap(msgdisplaybuf);
      if (MsgStrings[msgstr].tile!=0)
      {
	frame2x2(msgdisplaybuf,&QMisc,24,MsgStrings[msgstr].y,MsgStrings[msgstr].tile,MsgStrings[msgstr].cset,
		   26,5,0,0,0);
      }
      set_clip_state(msgdisplaybuf, 1);
      putprices(false);
      return true;
  }
  return false;
}

void putmsg()
{
  if (linkedmsgclk>0)
  {
    if (linkedmsgclk==1)
    {
      if (cAbtn()||cBbtn())
      {
      	msgstr=MsgStrings[msgstr].nextstring;
        linkedmsgclk=0;
        msgpos=msgptr=msgcset=0;
	msgcolour=1;
	msgspeed=5;
        clear_bitmap(msgdisplaybuf);
	if (!msgstr)
	{
	  msgfont=zfont;
	  blockpath=false;
	  goto disappear;
	}
	msgfont=setmsgfont();
	if (MsgStrings[msgstr].tile!=0)
	{
          frame2x2(msgdisplaybuf,&QMisc,24,MsgStrings[msgstr].y,MsgStrings[msgstr].tile,MsgStrings[msgstr].cset,
		   26,5,0,0,0);
	}
        set_clip_state(msgdisplaybuf, 1);
        putprices(false);
      }
    }
    else
    {
      --linkedmsgclk;
    }
  }
  if(!msgstr || msgpos>=72 || msgptr>=MSGSIZE)
    return;

  if(((cBbtn())&&(get_bit(quest_rules,qr_ALLOWMSGBYPASS)) || msgspeed==0))
  {
    //finish writing out the string
    for (;msgptr<MSGSIZE;++msgptr)
    {
      if (!parsemsgcode())
      {
        textprintf_ex(msgdisplaybuf,msgfont,((msgpos%24)<<3)+32,((msgpos/24)<<3)+(MsgStrings[msgstr].y)+8,CSET(msgcset)+msgcolour,-1,
                    "%c",MsgStrings[msgstr].s[msgptr]);
	msgpos++;
      }
    }
    msgclk=72;
    msgpos=72;
  }
  else
  {
    if(((msgclk++)%(msgspeed+1)<msgspeed)&&((!cAbtn())||(!get_bit(quest_rules,qr_ALLOWFASTMSG))))
      return;
  }

  if(msgptr == 0)
  {
    while(MsgStrings[msgstr].s[msgptr]==' ')
    {
      ++msgptr;
      ++msgpos;
    }
  }

  //using the clip value to indicate the bitmap is "dirty"
  //rather than add yet another global variable
  set_clip_state(msgdisplaybuf, 0);
  if (!parsemsgcode())
  {
    sfx(MsgStrings[msgstr].sfx);
    textprintf_ex(msgdisplaybuf,msgfont,((msgpos%24)<<3)+32,((msgpos/24)<<3)+(MsgStrings[msgstr].y)+8,CSET(msgcset)+msgcolour,-1,
                "%c",MsgStrings[msgstr].s[msgptr]);
    msgpos++;
  }
  msgptr++;

  if(MsgStrings[msgstr].s[msgptr]==' ' && MsgStrings[msgstr].s[msgptr+1]==' ')
    while(MsgStrings[msgstr].s[msgptr]==' ')
  {
      ++msgpos;
      ++msgptr;
  }

  if((msgpos>=72 || msgptr>=MSGSIZE) && !linkedmsgclk)
  {
    if (MsgStrings[msgstr].nextstring!=0 || get_bit(quest_rules,qr_MSGDISAPPEAR))
    {
      linkedmsgclk=51;
    }
    else
    {
disappear:
      //these are to cancel out any keys that Link may
      //be pressing so he doesn't attack at the end of
      //a message if he was scrolling through it quickly.
      rAbtn();
      rBbtn();

      Link.unfreeze();
      if(repaircharge)
      {
        //       if (get_bit(quest_rules,qr_REPAIRFIX)) {
        //         fixed_door=true;
        //       }

        game->change_drupy( -tmpscr[1].catchall);
      }

      if(adjustmagic)
      {
        if (game->get_magicdrainrate())
	  game->set_magicdrainrate( 1);
        sfx(WAV_SCALE);
        setmapflag();
      }
      if(learnslash)
      {
        game->set_canslash( 1);
        sfx(WAV_SCALE);
        setmapflag();
      }
    }
  }
}

void domoney()
{
  static bool sfxon = false;

  if(game->get_drupy()==0)
  {
    sfxon = false;
    return;
  }

  if(frame&1)
  {
    sfxon = true;
    if(game->get_drupy()>0)
    {
      /*int max = 255;
      if(current_item(itype_wallet,true)==1)
        max = 500;
      if(get_bit(quest_rules,qr_999R) || current_item(itype_wallet,true)==2)
        max = 999;*/

      if(game->get_rupies() < game->get_maxcounter(1))
      {
        game->change_rupies( 1);
        game->change_drupy( -1);
      }
      else game->set_drupy( 0);
    }
    else
    {
      if(game->get_rupies()>0)
      {
        game->change_rupies( -1);
        game->change_drupy( 1);
      }
      else game->set_drupy( 0);
    }
  }

  if(sfxon && !lensclk)
    sfx(WAV_MSG);
}

void domagic()                                              //basically a copy of domoney()
{
  if (magicdrainclk==32767)
  {
    magicdrainclk=-1;
  }
  ++magicdrainclk;

  static bool sfxon = false;

  if(game->get_dmagic()==0)
  {
    sfxon = false;
    return;
  }

  if (game->get_dmagic()>0)
  {
    if(frame&1)
    {
      sfxon = true;
      if(game->get_magic() < game->get_maxmagic())
      {
        game->change_magic( MAGICPERBLOCK/4);
        game->change_dmagic( -MAGICPERBLOCK/4);
      }
      else
      {
        game->set_dmagic( 0);
        game->set_magic( game->get_maxmagic());
      }
    }
    if(sfxon && !lensclk)
      sfx(WAV_MSG);
  }
  else
  {
    if(frame&1)
    {
      sfxon = true;
      if(game->get_magic()>0)
      {
        game->change_magic( -2*game->get_magicdrainrate());
        game->change_dmagic( 2*game->get_magicdrainrate());
      }
      else
      {
        game->set_dmagic( 0);
        game->set_magic( 0);
      }
    }
  }
}

/***  Collision detection & handling  ***/

void check_collisions()
{
  for(int i=0; i<Lwpns.Count(); i++)
  {
    weapon *w = (weapon*)Lwpns.spr(i);
    if(!(w->Dead()))
    {
      for(int j=0; j<guys.Count(); j++)
      {
        if(((enemy*)guys.spr(j))->hit(w))
        {
          int h = ((enemy*)guys.spr(j)) -> takehit(w->id,w->power,w->x,w->y,w->dir);
          if(h)
          {
            w->onhit(false);
          }
          if(h==2)
          {
            break;
          }
        }
        if(w->Dead())
        {
          break;
        }
      }
      if (get_bit(quest_rules,qr_Z3BRANG_HSHOT))
      {
        if(w->id == wBrang || w->id==wHookshot)
        {
          for(int j=0; j<items.Count(); j++)
          {
            if(items.spr(j)->hit(w))
            {
              if((((item*)items.spr(j))->pickup & ipTIMER && ((item*)items.spr(j))->clk2 >= 32)
		 || (get_bit(quest_rules,qr_BRANGPICKUP) && prices[j][0] == 0 && !(((item*)items.spr(j))->pickup & ipDUMMY)))
              {
                if (w->dragging==-1)
                {
                  w->dead=1;
                  if (w->id==wBrang&&w->dummy_bool[0]&&(w->misc==1))
                  {
                    add_grenade(w->x,w->y,w->z,current_item_power(itype_brang)>0,w->parentid);
                    w->dummy_bool[0]=false;
                  }
                  ((item*)items.spr(j))->clk2=256;
                  w->dragging=j;
                }
              }
            }
          }
        }
      }
      else
      {
        if(w->id == wBrang || w->id == wArrow || w->id==wHookshot)
        {
          for(int j=0; j<items.Count(); j++)
          {
            if(items.spr(j)->hit(w))
            {
              if((((item*)items.spr(j))->pickup & ipTIMER && ((item*)items.spr(j))->clk2 >= 32)
		|| (get_bit(quest_rules,qr_BRANGPICKUP) && prices[j][0] == 0))
              {
                if(itemsbuf[items.spr(j)->id].collect_script)
                {
                    run_script(itemsbuf[items.spr(j)->id].collect_script, j, SCRIPT_ITEM);
                }
                getitem(items.spr(j)->id);
                items.del(j);
                --j;
              }
            }
          }
        }
      }
    }
  }
}

// messy code to do the enemy-carrying-the-item thing

void dragging_item()
{
  if (get_bit(quest_rules,qr_Z3BRANG_HSHOT))
  {
    for(int i=0; i<Lwpns.Count(); i++)
    {
      weapon *w = (weapon*)Lwpns.spr(i);
      if(w->id == wBrang || w->id==wHookshot)
      {
        if (w->dragging>=0 && w->dragging<items.Count())
        {
          items.spr(w->dragging)->x=w->x;
          items.spr(w->dragging)->y=w->y;
        }
      }
    }
  }
}

void roaming_item()
{
  if(hasitem!=2 || !loaded_enemies)
    return;

  if(guys.Count()==0)                                       // Only case is when you clear all the guys and
  {                                                         // leave w/out getting item, then return to room.
    return;                                                 // We'll let LinkClass::checkspecial() handle it.
  }

  if(itemindex==-1)
  {
    guyindex = -1;
    for(int i=0; i<guys.Count(); i++)
    {
      if(((enemy*)guys.spr(i))->itemguy)
      {
        guyindex = i;
      }
    }
    if (guyindex == -1)                                     //This happens when "default enemies" such as
    {
      return;                                               //eSHOOTFBALL are alive but enemies from the list
    }                                                       //are not. Defer to LinkClass::checkspecial().

    int Item=tmpscr->item;
    /*if(getmapflag())
    {
      Item=0;
    }*/
    if(!getmapflag() && (tmpscr->hasitem != 0))
    {
      itemindex=items.Count();
      additem(0,0,Item,ipENEMY+ipONETIME+ipBIGRANGE
              + ( ((tmpscr->flags3&fHOLDITEM) || (Item==iTriforce)) ? ipHOLDUP : 0)
             );
    }
    else
    {
      hasitem=0;
      return;
    }
  }
  if (get_bit(quest_rules,qr_HIDECARRIEDITEMS))
  {
    items.spr(itemindex)->x = -1000;
    items.spr(itemindex)->y = -1000;
  }
  else
  {
    items.spr(itemindex)->x = guys.spr(guyindex)->x;
    items.spr(itemindex)->y = guys.spr(guyindex)->y - 2;
  }
}

char *old_guy_string[OLDMAXGUYS] =
{
  "(None)","Abei","Ama","Merchant","Moblin","Fire","Fairy","Goriya","Zelda","Abei 2","Empty","","","","","","","","","",
  // 020
  "Octorok (L1, Slow)","Octorok (L2, Slow)","Octorok (L1, Fast)","Octorok (L2, Fast)","Tektite (L1)",
  // 025
  "Tektite (L2)","Leever (L1)","Leever (L2)","Moblin (L1)","Moblin (L2)",
  // 030
  "Lynel (L1)","Lynel (L2)","Peahat (L1)","Zora <Slide-Proof>","Rock <Slide-Proof>",
  // 035
  "Ghini (L1, Normal)","Ghini (L1, Phantom)","Armos","Keese (CSet 7)","Keese (CSet 8)",
  // 040
  "Keese (CSet 9)","Stalfos (L1)","Gel (L1, Normal)","Zol (L1, Normal)","Rope (L1)",
  // 045
  "Goriya (L1)","Goriya (L2)","Trap (4-Way) ","Wall Master","Darknut (L1)",
  // 050
  "Darknut (L2)","Bubble (Sword, Temporary Disabling)","Vire (Normal)","Like Like","Gibdo",
  // 055
  "Pols Voice (Arrow)","Wizzrobe (Teleporting)","Wizzrobe (Floating)","Aquamentus (Facing Left)","Moldorm",
  // 060
  "Dodongo","Manhandla (L1)","Gleeok (1 Head)","Gleeok (2 Heads)","Gleeok (3 Heads)",
  // 065
  "Gleeok (4 Heads)","Digdogger (1 Kid)","Digdogger (3 Kids)","Digdogger Kid (1)","Digdogger Kid (2)",
  // 070
  "Digdogger Kid (3)","Digdogger Kid (4)","Gohma (L1)","Gohma (L2)","Lanmola (L1)",
  // 075
  "Lanmola (L2)","Patra (Big Circle)","Patra (Oval)","Ganon","Stalfos (L2)",
  // 080
  "Rope (L2)","Bubble (Sword, Permanent Disabling)","Bubble (Sword, Re-enabling)","Shooter (Fireball)","Item Fairy ",
  // 085
  "Fire","Octorok (Magic)", "Darknut (Death Knight)", "Gel (L1, Tribble)", "Zol (L1, Tribble)",
  // 090
  "Keese (Tribble)", "Vire (Tribble)", "Darknut (Splitting)", "Aquamentus (Facing Right)", "Manhandla (L2)",
  // 095
  "Trap (Horizontal, Line of Sight) ", "Trap (Vertical, Line of Sight) ", "Trap (Horizontal, Constant) ", "Trap (Vertical, Constant) ", "Wizzrobe (Fire)",
  // 100
  "Wizzrobe (Wind)", "Ceiling Master ", "Floor Master ", "Patra (BS Zelda)", "Patra (L2)",
  // 105
  "Patra (L3)", "Bat", "Wizzrobe (Bat)", "Wizzrobe (Bat 2) ", "Gleeok (Fire, 1 Head)",
  // 110
  "Gleeok (Fire, 2 Heads)",  "Gleeok (Fire, 3 Heads)","Gleeok (Fire, 4 Heads)", "Wizzrobe (Mirror)", "Dodongo (BS Zelda)",
  // 115
  "Dodongo (Fire) ","Trigger", "Bubble (Item, Temporary Disabling)", "Bubble (Item, Permanent Disabling)", "Bubble (Item, Re-enabling)",
  // 120
  "Stalfos (L3)", "Gohma (L3)", "Gohma (L4)", "NPC 1 (Standing) ", "NPC 2 (Standing) ",
  // 125
  "NPC 3 (Standing) ", "NPC 4 (Standing) ", "NPC 5 (Standing) ", "NPC 6 (Standing) ", "NPC 1 (Walking) ",
  // 130
  "NPC 2 (Walking) ", "NPC 3 (Walking) ", "NPC 4 (Walking) ", "NPC 5 (Walking) ", "NPC 6 (Walking) ",
  // 135
  "Boulder <Slide-Proof>", "Goriya (L3)", "Leever (L3)", "Octorok (L3, Slow)", "Octorok (L3, Fast)",
  // 140
  "Octorok (L4, Slow)", "Octorok (L4, Fast)", "Trap (8-Way) ", "Trap (Diagonal) ", "Trap (/, Constant) ",
  // 145
  "Trap (/, Line of Sight) ", "Trap (\\, Constant) ", "Trap (\\, Line of Sight) ", "Trap (CW, Constant) ", "Trap (CW, Line of Sight) ",
  // 150
  "Trap (CCW, Constant) ", "Trap (CCW, Line of Sight) ", "Summoner", "Wizzrobe (Ice) ", "Shooter (Magic)",
  // 155
  "Shooter (Rock)", "Shooter (Spear)", "Shooter (Sword)", "Shooter (Fire)", "Shooter (Fire 2)",
  // 160
  "Bombchu", "Gel (L2, Normal)", "Zol (L2, Normal)", "Gel (L2, Tribble)", "Zol (L2, Tribble)",
  // 165
  "Tektite (L3) ", "Spinning Tile (Immediate) ", "Spinning Tile (Random) ", "Lynel (L3) ", "Peahat (L2) ",
  // 170
  "Pols Voice (Magic) ", "Pols Voice (Whistle) ", "Darknut (Mirror) ", "Ghini (L2, Fire) ", "Ghini (L2, Magic) ",
  // 175
  "Grappler Bug (HP) ", "Grappler Bug (MP) "
};

char *guy_string[eMAXGUYS];

/*** end of guys.cc ***/
