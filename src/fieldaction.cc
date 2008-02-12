/*
 * Copyright (C) 2002-2004, 2006-2008 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "fieldaction.h"

#include "attack_controller.h"
#include "interactive_player.h"
#include "cmd_queue.h"
#include "editorinteractive.h"
#include "game_debug_ui.h"
#include "graphic.h"
#include "i18n.h"
#include "maphollowregion.h"
#include "militarysite.h"
#include "overlay_manager.h"
#include "player.h"
#include "soldier.h"
#include "transport.h"
#include "tribe.h"
#include "warehouse.h"
#include "watchwindow.h"

#include "ui_box.h"
#include "ui_button.h"
#include "ui_icongrid.h"
#include "ui_tabpanel.h"
#include "ui_textarea.h"
#include "ui_unique_window.h"

#include "upcast.h"

namespace Widelands {struct Building_Descr;};
using Widelands::Building;
using Widelands::Game;

#define BG_CELL_WIDTH  34 // extents of one cell
#define BG_CELL_HEIGHT 34


// The BuildGrid presents a selection of buildable buildings
struct BuildGrid : public UI::Icon_Grid {
	BuildGrid
		(UI::Panel* parent,
		 Widelands::Tribe_Descr const & tribe,
		 const int32_t x, const int32_t y,
		 int32_t cols);

	UI::Signal1<int32_t> buildclicked;
	UI::Signal1<int32_t> buildmouseout;
	UI::Signal1<int32_t> buildmousein;

	void add(int32_t id);

private:
	void clickslot(int32_t idx);
	void mouseoutslot(int32_t idx);
	void mouseinslot(int32_t idx);

private:
	Widelands::Tribe_Descr const & m_tribe;
};


/*
===============
BuildGrid::BuildGrid

Initialize the grid
===============
*/
BuildGrid::BuildGrid
(UI::Panel* parent,
 Widelands::Tribe_Descr const & tribe,
 const int32_t x, const int32_t y,
 int32_t cols)
:
UI::Icon_Grid(parent, x, y, BG_CELL_WIDTH, BG_CELL_HEIGHT, Grid_Horizontal, cols),
m_tribe(tribe)
{
	clicked.set(this, &BuildGrid::clickslot);
	mouseout.set(this, &BuildGrid::mouseoutslot);
	mousein.set(this, &BuildGrid::mouseinslot);
}


/*
===============
BuildGrid::add

Add a new building to the list of buildable buildings
===============
*/
void BuildGrid::add(int32_t id)
{
	Widelands::Building_Descr const & descr = *m_tribe.get_building_descr(id);
	UI::Icon_Grid::add
		(descr.get_buildicon(), reinterpret_cast<void *>(id), descr.descname());
}


/*
===============
BuildGrid::clickslot [private]

The icon with the given index has been clicked. Figure out which building it
belongs to and trigger signal buildclicked.
===============
*/
void BuildGrid::clickslot(int32_t idx)
{
	buildclicked.call
		(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}


/*
===============
BuildGrid::mouseoutslot [private]

The mouse pointer has left the icon with the given index. Figure out which
building it belongs to and trigger signal buildmouseout.
===============
*/
void BuildGrid::mouseoutslot(int32_t idx)
{
	buildmouseout.call
		(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}


/*
===============
BuildGrid::mouseinslot [private]

The mouse pointer has entered the icon with the given index. Figure out which
building it belongs to and trigger signal buildmousein.
===============
*/
void BuildGrid::mouseinslot(int32_t idx)
{
	buildmousein.call
		(static_cast<int32_t>(reinterpret_cast<intptr_t>(get_data(idx))));
}



/*
==============================================================================

FieldActionWindow IMPLEMENTATION

==============================================================================
*/
struct FieldActionWindow : public UI::UniqueWindow {
	FieldActionWindow
		(Interactive_Base           * iabase,
		 Widelands::Player          * plr,
		 UI::UniqueWindow::Registry * registry);
	~FieldActionWindow();

	void init();
	void add_buttons_auto();
	void add_buttons_build(int32_t buildcaps);
	void add_buttons_road(bool flag);
	void add_buttons_attack();

	void act_watch();
	void act_show_census();
	void act_show_statistics();
	void act_debug();
	void act_buildflag();
	void act_ripflag();
	void act_buildroad();
	void act_abort_buildroad();
	void act_removeroad();
	void act_build(int32_t idx);
	void building_icon_mouse_out(int32_t idx);
	void building_icon_mouse_in(int32_t idx);
	void act_geologist();
	void act_attack();         /// Launch the attack
	void act_attack_more();    /// Increase the number of soldiers to be launched
	void act_attack_less();    /// Decrease the number of soldiers to be launched
	void act_attack_strong();  /// Prepare to launch strongest soldiers
	void act_attack_weak();    /// Prepare to launch weakest soldiers
	uint32_t get_max_attackers();  /// Total number of attackers available for a specific enemy flag

private:
	uint32_t add_tab
		(const char * picname,
		 UI::Panel * panel,
		 const std::string & tooltip_text = std::string());
	void add_button
		(UI::Box *,
		 char const * picname,
		 void (FieldActionWindow::*fn)(),
		 std::string const & tooltip_text);
	void okdialog();

	Interactive_Base    *m_iabase;
	Widelands::Player * m_plr;
	Widelands::Map    * m_map;
	Overlay_Manager & m_overlay_manager;

	Widelands::FCoords  m_field;

	UI::Tab_Panel    * m_tabpanel;
	bool m_fastclick; // if true, put the mouse over first button in first tab
	uint32_t m_best_tab;
	Overlay_Manager::Job_Id m_workarea_preview_job_id;
	uint32_t workarea_cumulative_picid[NUMBER_OF_WORKAREA_PICS];

	/// Variables to use with attack dialog.
	UI::Textarea * m_text_attackers;
	uint32_t       m_attackers;      //  0 - Number of available soldiers.
	int32_t        m_attackers_type; //  STRONGEST - WEAKEST ...
};

static const char* const pic_tab_buildroad = "pics/menu_tab_buildroad.png";
static const char* const pic_tab_watch = "pics/menu_tab_watch.png";
static const char* const pic_tab_buildhouse[] = {
	"pics/menu_tab_buildsmall.png",
	"pics/menu_tab_buildmedium.png",
	"pics/menu_tab_buildbig.png"
};
static const std::string tooltip_tab_build[] = {
	_("Build small buildings"),
	_("Build medium buildings"),
	_("Build large buildings")
};

static const char* const pic_tab_buildmine = "pics/menu_tab_buildmine.png";

static const char* const pic_buildroad = "pics/menu_build_way.png";
static const char* const pic_remroad = "pics/menu_rem_way.png";
static const char* const pic_buildflag = "pics/menu_build_flag.png";
static const char* const pic_ripflag = "pics/menu_rip_flag.png";
static const char* const pic_watchfield = "pics/menu_watch_field.png";
static const char* const pic_showcensus = "pics/menu_show_census.png";
static const char* const pic_showstatistics = "pics/menu_show_statistics.png";
static const char* const pic_debug = "pics/menu_debug.png";
static const char* const pic_abort = "pics/menu_abort.png";
static const char* const pic_geologist = "pics/menu_geologist.png";

/// TESTING STUFF
static const char* const pic_tab_attack    = "pics/menu_tab_attack.png";
static const char* const pic_attack_more   = "pics/attack_add_soldier.png";
static const char* const pic_attack_less   = "pics/attack_sub_soldier.png";
static const char* const pic_attack_strong = "pics/attack_strongest.png";
static const char* const pic_attack_weak   = "pics/attack_weakest.png";
static const char* const pic_attack        = "pics/menu_attack.png";


/*
===============
FieldActionWindow::FieldActionWindow

Initialize a field action window, creating the appropriate buttons.
===============
*/
FieldActionWindow::FieldActionWindow
(Interactive_Base           * const iabase,
 Widelands::Player          * const plr,
 UI::UniqueWindow::Registry * const registry) :
	UI::UniqueWindow(iabase, registry, 68, 34, _("Action")),
	m_iabase(iabase),
	m_plr(plr),
	m_map(&iabase->egbase().map()),
	m_overlay_manager(*m_map->get_overlay_manager()),
	m_field(iabase->get_sel_pos().node, &(*m_map)[iabase->get_sel_pos().node]),
	m_best_tab(0),
	m_workarea_preview_job_id(Overlay_Manager::Job_Id::Null())
{


	iabase->set_sel_freeze(true);

	m_tabpanel = new UI::Tab_Panel(this, 0, 0, 1);
	m_tabpanel->set_snapparent(true);
	m_text_attackers = 0;

	m_fastclick = true;
	char filename[] = "pics/workarea0cumulative.png";
	compile_assert(NUMBER_OF_WORKAREA_PICS <= 9);
	for (Workarea_Info::size_type i = 0; i < NUMBER_OF_WORKAREA_PICS; ++i) {
		++filename[13];
		workarea_cumulative_picid[i] = g_gr->get_picture(PicMod_Game, filename);
	}
}

/*
===============
FieldActionWindow::~FieldActionWindow

Free allocated resources, remove from registry.
===============
*/
FieldActionWindow::~FieldActionWindow()
{
	if (m_workarea_preview_job_id)
		m_overlay_manager.remove_overlay(m_workarea_preview_job_id);
	m_iabase->set_sel_freeze(false);
	delete m_text_attackers;
	m_text_attackers = 0;
}


/*
===============
FieldActionWindow::init

Initialize after buttons have been registered.
This mainly deals with mouse placement
===============
*/
void FieldActionWindow::init()
{
	m_tabpanel->resize();

	center_to_parent(); // override UI::UniqueWindow position

	// Move the window away from the current mouse position, i.e.
	// where the field is, to allow better view
	const Point mouse = get_mouse_position();
	if
		(0 <= mouse.x  and mouse.x < get_w()
		 and
		 0 <= mouse.y and mouse.y < get_h())
	{
		set_pos
		(Point(get_x(), get_y())
		 +
		 Point(0, (mouse.y < get_h() / 2 ? 1 : -1) * get_h()));
		move_inside_parent();
	}

	// Now force the mouse onto the first button
	// TODO: should be on first tab button if we're building
	set_mouse_pos
		(Point(17 + BG_CELL_WIDTH * m_best_tab, m_fastclick ? 51 : 17));
}


/*
===============
FieldActionWindow::add_buttons_auto

Add the buttons you normally get when clicking on a field.
===============
*/
void FieldActionWindow::add_buttons_auto()
{
	UI::Box* buildbox = 0;
	UI::Box* watchbox;

	watchbox = new UI::Box(m_tabpanel, 0, 0, UI::Box::Horizontal);

	// Add road-building actions
	if (m_field.field->get_owned_by() == m_plr->get_player_number()) {

		Widelands::BaseImmovable * const imm = m_map->get_immovable(m_field);

		// The box with road-building buttons
		buildbox = new UI::Box(m_tabpanel, 0, 0, UI::Box::Horizontal);

		if (upcast(Widelands::Flag, flag, imm)) {
			// Add flag actions
			add_button(buildbox, pic_buildroad, &FieldActionWindow::act_buildroad, _("Build road"));

			Building * const building = flag->get_building();

			if
				(!building
				 ||
				 building->get_playercaps() & (1 << Building::PCap_Bulldoze))
				add_button(buildbox, pic_ripflag, &FieldActionWindow::act_ripflag, _("Destroy this flag"));

			if (dynamic_cast<const Game *>(&m_iabase->egbase()))
				add_button
					(buildbox,
					 pic_geologist,
					 &FieldActionWindow::act_geologist,
					 _("Send geologist to explore site"));
			// No geologist in editor
		}
		else
		{
			int32_t buildcaps = m_plr->get_buildcaps(m_field);

			// Add house building
			if
				((buildcaps & Widelands::BUILDCAPS_SIZEMASK)
				 ||
				 (buildcaps & Widelands::BUILDCAPS_MINE))
				add_buttons_build(buildcaps);

			// Add build actions
			if (buildcaps & Widelands::BUILDCAPS_FLAG)
				add_button(buildbox, pic_buildflag, &FieldActionWindow::act_buildflag, _("Put a flag"));

			if (dynamic_cast<Widelands::Road const *>(imm))
				add_button(buildbox, pic_remroad, &FieldActionWindow::act_removeroad, _("Destroy a road"));
		}
	} else add_buttons_attack ();

	//  Watch actions, only when game (no use in editor) same for statistics.
	//  census is ok
	if (dynamic_cast<const Game *>(&m_iabase->egbase())) {
		add_button
			(watchbox,
			 pic_watchfield,
			 &FieldActionWindow::act_watch,
			 _("Watch field in a separate window"));
		add_button
			(watchbox,
			 pic_showstatistics,
			 &FieldActionWindow::act_show_statistics,
			 _("Toggle building statistics display"));
	}
	add_button(watchbox, pic_showcensus, &FieldActionWindow::act_show_census, _("Toggle building label display"));

	if (m_iabase->get_display_flag(Interactive_Base::dfDebug))
		add_button(watchbox, pic_debug, &FieldActionWindow::act_debug, _("Debug window"));

	// Add tabs
	if (buildbox && buildbox->get_nritems()) {
		buildbox->resize();
		add_tab(pic_tab_buildroad, buildbox, _("Build roads"));
	}

	watchbox->resize();
	add_tab(pic_tab_watch, watchbox, _("Watch"));
}

void FieldActionWindow::add_buttons_attack ()
{
	UI::Box * attackbox = 0;

	// Add attack button
	if (m_field.field->get_owned_by() != m_plr->get_player_number()) {

		// The box with attack buttons
		attackbox = new UI::Box(m_tabpanel, 0, 0, UI::Box::Horizontal);

		if (upcast(Building, building, m_map->get_immovable(m_field)))
			if
				(dynamic_cast<const Game *>(&m_iabase->egbase())
				 and
				 (dynamic_cast<Widelands::MilitarySite const *>(building)
				  or
				  dynamic_cast<Widelands::Warehouse    const *>(building)))
			{
				m_attackers = 0;
				m_attackers_type = STRONGEST;
				add_button
					(attackbox,
					 pic_attack_less,
					 &FieldActionWindow::act_attack_less,
					 _("Send less soldiers"));

				m_text_attackers =
					new UI::Textarea(attackbox, 90, 0, "000/000", Align_Center);
				attackbox->add(m_text_attackers, UI::Box::AlignTop);

				add_button
					(attackbox,
					 pic_attack_more,
					 &FieldActionWindow::act_attack_more,
					 _("Send more soldiers"));

				add_button
					(attackbox,
					 pic_attack_strong,
					 &FieldActionWindow::act_attack_strong,
					 _("Most agressive attack"));
				add_button
					(attackbox,
					 pic_attack_weak,
					 &FieldActionWindow::act_attack_weak,
					 _("Cautious attack"));

				add_button
					(attackbox,
					 pic_attack,
					 &FieldActionWindow::act_attack,
					 _("Start attack"));
				act_attack_more();
			}
	}

	if (attackbox and attackbox->get_nritems()) { //  add tab
		attackbox->resize();
		add_tab(pic_tab_attack, attackbox, _("Attack"));
	}
}

/*
===============
FieldActionWindow::add_buttons_build

Add buttons for house building.
===============
*/
void FieldActionWindow::add_buttons_build(int32_t buildcaps)
{
	BuildGrid* bbg_house[3] = {0, 0, 0};
	BuildGrid* bbg_mine = 0;

	Widelands::Tribe_Descr const & tribe = m_plr->tribe();

	m_fastclick = false;

	for (int32_t id = 0; id < tribe.get_nrbuildings(); ++id) {
		Widelands::Building_Descr const & descr = *tribe.get_building_descr(id);
		BuildGrid** ppgrid;

		//  Some buildings cannot be built (i.e. construction site, HQ) and not
		//  allowed buildings. The rules are different in editor and game:
		//  enhanced buildings _are_ buildable in the editor
		if (dynamic_cast<const Game *>(&m_iabase->egbase())) {
			if (!descr.get_buildable() || !m_plr->is_building_allowed(id))
				continue;
		} else if (!descr.get_buildable() && !descr.get_enhanced_building())
			continue;

		// Figure out if we can build it here, and in which tab it belongs
		if (descr.get_ismine()) {
			if (!(buildcaps & Widelands::BUILDCAPS_MINE))
				continue;

			ppgrid = &bbg_mine;
		} else {
			int32_t size = descr.get_size() - Widelands::BaseImmovable::SMALL;

			if ((buildcaps & Widelands::BUILDCAPS_SIZEMASK) < (size+1))
				continue;

			ppgrid = &bbg_house[size];
		}

		// Allocate the tab's grid if necessary
		if (!*ppgrid) {
			*ppgrid = new BuildGrid(m_tabpanel, tribe, 0, 0, 5);
			(*ppgrid)->buildclicked.set(this, &FieldActionWindow::act_build);
			(*ppgrid)->buildmouseout.set(this, &FieldActionWindow::building_icon_mouse_out);
			(*ppgrid)->buildmousein.set(this, &FieldActionWindow::building_icon_mouse_in);
		}

		// Add it to the grid
		(*ppgrid)->add(id);
	}

	// Add all necessary tabs
	for (int32_t i = 0; i < 3; ++i)
		if (bbg_house[i])
			m_tabpanel->activate
				(m_best_tab = add_tab
				 (pic_tab_buildhouse[i],
				  bbg_house[i],
				  i18n::translate(tooltip_tab_build[i])));

	if (bbg_mine)
		m_tabpanel->activate
			(m_best_tab = add_tab(pic_tab_buildmine, bbg_mine, _("Build mines")));
}


/*
===============
FieldActionWindow::add_buttons_road

Buttons used during road building: Set flag here and Abort
===============
*/
void FieldActionWindow::add_buttons_road(bool flag)
{
	UI::Box* buildbox = new UI::Box(m_tabpanel, 0, 0, UI::Box::Horizontal);

	if (flag)
		add_button(buildbox, pic_buildflag, &FieldActionWindow::act_buildflag, _("Build flag"));

	add_button(buildbox, pic_abort, &FieldActionWindow::act_abort_buildroad, _("Cancel road"));

	// Add the box as tab
	buildbox->resize();
	add_tab(pic_tab_buildroad, buildbox, _("Build roads"));
}


/*
===============
FieldActionWindow::add_tab

Convenience function: Adds a new tab to the main tab panel
===============
*/
uint32_t FieldActionWindow::add_tab
(const char * picname, UI::Panel * panel, const std::string & tooltip_text)
{
	return m_tabpanel->add
		(g_gr->get_picture(PicMod_Game, picname), panel, tooltip_text);
}


/*
===============
FieldActionWindow::add_button
===============
*/
void FieldActionWindow::add_button(UI::Box* box, const char* picname, void (FieldActionWindow::*fn)(), const std::string & tooltip_text)
{
	box->add
		(new UI::Button<FieldActionWindow>
		 (box,
		  0, 0, 34, 34,
		  2,
		  g_gr->get_picture(PicMod_Game, picname),
		  fn, this, tooltip_text),
		 UI::Box::AlignTop);
}

/*
===============
FieldActionWindow::okdialog

Call this from the button handlers.
It resets the mouse to its original position and closes the window
===============
*/
void FieldActionWindow::okdialog()
{
	m_iabase->warp_mouse_to_field(m_field);
	die();
}

/*
===============
FieldActionWindow::act_watch()

Open a watch window for the given field and delete self.
===============
*/
void FieldActionWindow::act_watch()
{
	assert(dynamic_cast<const Game *>(&m_iabase->egbase()));

	show_watch_window(static_cast<Interactive_Player &>(*m_iabase), m_field);
	okdialog();
}


/*
===============
FieldActionWindow::act_show_census
FieldActionWindow::act_show_statistics

Toggle display of census and statistics for buildings, respectively.
===============
*/
void FieldActionWindow::act_show_census()
{
	m_iabase->set_display_flag
		(Interactive_Base::dfShowCensus,
		 !m_iabase->get_display_flag(Interactive_Base::dfShowCensus));
	okdialog();
}

void FieldActionWindow::act_show_statistics()
{
	m_iabase->set_display_flag
		(Interactive_Base::dfShowStatistics,
		 !m_iabase->get_display_flag(Interactive_Base::dfShowStatistics));
	okdialog();
}


/*
===============
FieldActionWindow::act_debug

Show a debug widow for this field.
===============
*/
void FieldActionWindow::act_debug()
{
	show_field_debug(*m_iabase, m_field);
}


/*
===============
FieldActionWindow::act_buildflag

Build a flag at this field
===============
*/
void FieldActionWindow::act_buildflag()
{
	// Game: send command
	if (upcast(Game, game, &m_iabase->egbase()))
		game->send_player_build_flag(m_plr->get_player_number(), m_field);
	// Editor: Just plain build this flag
	else m_plr->build_flag(m_field);
	if (m_iabase->is_building_road())
		m_iabase->finish_build_road();
	else
		m_iabase->set_flag_to_connect(m_field);
	okdialog();
}

/*
===============
FieldActionWindow::act_ripflag

Remove the flag at this field
===============
*/
void FieldActionWindow::act_ripflag()
{
	okdialog();
	Widelands::Editor_Game_Base & egbase = m_iabase->egbase();
	if (upcast(Widelands::Flag, flag, m_field.field->get_immovable()))
		if (Building * const building = flag->get_building()) {
			if (building->get_playercaps() & (1 << Building::PCap_Bulldoze))
				show_bulldoze_confirm(m_iabase, building, flag);
		} else {
			if (upcast(Game, game, &egbase))
				game->send_player_bulldoze (flag);
			else // Editor
				flag->remove(&egbase);
			m_iabase->need_complete_redraw();
		}
}


/*
===============
Start road building.
===============
*/
void FieldActionWindow::act_buildroad()
{
	m_iabase->start_build_road(m_field, m_plr->get_player_number());
	okdialog();
}

/*
===============
Abort building a road.
===============
*/
void FieldActionWindow::act_abort_buildroad()
{
	if (!m_iabase->is_building_road())
		return;

	m_iabase->abort_build_road();
	okdialog();
}

/*
===============
FieldActionWindow::act_removeroad

Remove the road at the given field
===============
*/
void FieldActionWindow::act_removeroad()
{
	Widelands::Editor_Game_Base & egbase = m_iabase->egbase();
	if (upcast(Widelands::Road, road, egbase.map().get_immovable(m_field))) {
		if (upcast(Game, game, &egbase))
			game->send_player_bulldoze(road);
		else
			road->owner().bulldoze(road);
	}
	m_iabase->need_complete_redraw();
	okdialog();
}


/*
===============
Start construction of the building with the give description index
===============
*/
void FieldActionWindow::act_build(int32_t idx)
{
	Widelands::Editor_Game_Base & egbase = m_iabase->egbase();
	if (upcast(Game, game, &egbase))
		game->send_player_build
		(static_cast<Interactive_Player*>(m_iabase)->get_player_number(),
		 m_field,
		 idx);
	else egbase.warp_building(m_field, m_plr->get_player_number(), idx);
	m_iabase->reference_player_tribe
		(m_plr->get_player_number(), &m_plr->tribe());
	m_iabase->set_flag_to_connect(egbase.map().br_n(m_field));
	okdialog();
}


/*
===============
FieldActionWindow::building_icon_mouse_out

The mouse pointer has moved away from the icon for the building with the index idx.
===============
*/
void FieldActionWindow::building_icon_mouse_out(int32_t) {
	if (m_workarea_preview_job_id) {
		m_overlay_manager.remove_overlay(m_workarea_preview_job_id);
		m_workarea_preview_job_id = Overlay_Manager::Job_Id::Null();
	}
}


/*
===============
FieldActionWindow::building_icon_mouse_in

The mouse pointer has moved to the icon for the building with the index idx.
===============
*/
void FieldActionWindow::building_icon_mouse_in(int32_t idx) {
	if (m_iabase->m_show_workarea_preview) {
		assert(not m_workarea_preview_job_id);
		m_workarea_preview_job_id = m_overlay_manager.get_a_job_id();
		Widelands::HollowArea<> hollow_area(Widelands::Area<>(m_field, 0), 0);
		const Workarea_Info & workarea_info =
			m_plr->tribe().get_building_descr(idx)->m_recursive_workarea_info;
		Workarea_Info::const_iterator it = workarea_info.begin();
		for
			(Workarea_Info::size_type i =
			 std::min(workarea_info.size(), NUMBER_OF_WORKAREA_PICS);
			 i;
			 ++it)
		{
			--i;
			hollow_area.radius = it->first;
			assert(hollow_area.radius);
			assert(hollow_area.hole_radius < hollow_area.radius);
			Widelands::MapHollowRegion<> mr(*m_map, hollow_area);
			do m_overlay_manager.register_overlay
				(mr.location(),
				 workarea_cumulative_picid[i],
				 0,
				 Point::invalid(),
				 m_workarea_preview_job_id);
			while (mr.advance(*m_map));
			hollow_area.hole_radius = hollow_area.radius;
		}

#if 0
		//  This is debug output.
		//  Improvement suggestion: add to sign explanation window instead.
		for
			(Workarea_Info::const_iterator it = workarea_info.begin();
			 it != workarea_info.end();
			 ++it)
		{
			const int32_t radius = it->first;
			log("Radius: %i\n", radius);
			const std::set<std::string> & descriptions = it->second;
			for
				(std::set<std::string>::const_iterator de = descriptions.begin();
				 de != descriptions.end();
				 ++de)
				log("        %s\n", (*de).c_str());
		}
#endif

	}
}


/*
===============
FieldActionWindow::act_geologist

Call a geologist on this flag.
===============
*/
void FieldActionWindow::act_geologist()
{
	Game & game = dynamic_cast<Game &>(m_iabase->egbase());
	if (upcast(Widelands::Flag, flag, game.map().get_immovable(m_field)))
		game.send_player_flagaction (flag, Widelands::FLAGACTION_GEOLOGIST);

	okdialog();
}

/**
 * Here there are a problem: the sender of an event is allways the owner of were is done this even.
 * But for attacks, the owner of an event is the player who start an attack, so is needed to get an
 * extra parameter to the send_player_enemyflagaction, the player number
 *
 */
void FieldActionWindow::act_attack ()
{
	Game & game = dynamic_cast<Game &>(m_iabase->egbase());

	if (upcast(Building, b, game.map().get_immovable(m_field)))
		if (upcast(Widelands::Flag const, flag, b->get_base_flag()))
			if (m_attackers > 0)
				game.send_player_enemyflagaction
					(flag,
					 Widelands::ENEMYFLAGACTION_ATTACK,
					 dynamic_cast<Interactive_Player const &>(*m_iabase)
					 .get_player_number(),
					 m_attackers,       //  number of soldiers
					 m_attackers_type); //  type of soldiers

	okdialog();
}

void FieldActionWindow::act_attack_more() {
	char buf[20];
	uint32_t available = get_max_attackers();

	m_attackers = m_attackers < available ? m_attackers + 1 : available;

	sprintf(buf, "%d/%d", m_attackers, available);
	m_text_attackers->set_text (buf);
}

uint32_t FieldActionWindow::get_max_attackers() {
	return getMaxAttackSoldiers
		(m_iabase->egbase(),
		 dynamic_cast<Widelands::Flag const &>
		 (*dynamic_cast<Building &>(*m_map->get_immovable(m_field))
		  .get_base_flag()),
		 m_plr->get_player_number());
}

void FieldActionWindow::act_attack_less() {
	char buf[20];
	uint32_t available = get_max_attackers();

	m_attackers = m_attackers > 0 ? m_attackers - 1 : 0;

	sprintf(buf, "%d/%d", m_attackers, available);
	m_text_attackers->set_text (buf);
}

void FieldActionWindow::act_attack_strong()
{
	m_attackers_type = STRONGEST;
}


void FieldActionWindow::act_attack_weak()
{
	m_attackers_type = WEAKEST;
}

/*
===============
show_field_action

Perform a field action (other than building options).
Bring up a field action window or continue road building.
===============
*/
void show_field_action
(Interactive_Base           * const iabase,
 Widelands::Player          * const player,
 UI::UniqueWindow::Registry * const registry)
{
	FieldActionWindow *faw;

	// Force closing of old fieldaction windows. This is necessary because
	// show_field_action() does not always open a FieldActionWindow (e.g.
	// connecting the road we are building to an existing flag)
		delete registry->window;
	*registry = UI::UniqueWindow::Registry();

	if (!iabase->is_building_road()) {
		faw = new FieldActionWindow(iabase, player, registry);
		faw->add_buttons_auto();
		faw->init();
		return;
	}

	Widelands::Map const & map = player->egbase().map();

	// we're building a road right now
	Widelands::FCoords const target =
		map.get_fcoords(iabase->get_sel_pos().node);

	// if user clicked on the same field again, build a flag
	if (target == iabase->get_build_road_end()) {
		faw = new FieldActionWindow(iabase, player, registry);

		bool flag = false;
		if
			(target != iabase->get_build_road_start()
			 and
			 player->get_buildcaps(target) & Widelands::BUILDCAPS_FLAG)
			flag = true;
		faw->add_buttons_road(flag);
		faw->init();
		return;
	}

	// append or take away from the road
	if (!iabase->append_build_road(target)) {
		faw = new FieldActionWindow(iabase, player, registry);
		faw->add_buttons_road(false);
		faw->init();
		return;
	}

	// did he click on a flag or a road where a flag can be built?

	if (upcast(Widelands::PlayerImmovable const, i, map.get_immovable(target)))
	{
		bool finish = false;
		if      (dynamic_cast<Widelands::Flag const *>(i))
			finish = true;
		else if (dynamic_cast<Widelands::Road const *>(i))
			if (player->get_buildcaps(target) & Widelands::BUILDCAPS_FLAG) {
				if (upcast(Game, game, &iabase->egbase()))
					game->send_player_build_flag
						(player->get_player_number(), target);
				finish = true;
			}
		if (finish)
			iabase->finish_build_road();
	}
}
