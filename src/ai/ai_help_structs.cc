/*
 * Copyright (C) 2009-2010 by the Widelands Development Team
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "ai/ai_help_structs.h"

#include "base/macros.h"
#include "base/time_string.h"
#include "logic/map.h"
#include "logic/map.h"
#include "logic/player.h"

// couple of constants for calculation of road interconnections
constexpr int kRoadNotFound = -1000;
constexpr int kShortcutWithinSameEconomy = 1000;
constexpr int kRoadToDifferentEconomy = 10000;



namespace Widelands {

// CheckStepRoadAI
CheckStepRoadAI::CheckStepRoadAI(Player* const pl, uint8_t const mc, bool const oe)
   : player(pl), movecaps(mc), open_end(oe) {
}

bool CheckStepRoadAI::allowed(
   Map& map, FCoords start, FCoords end, int32_t, CheckStep::StepId const id) const {
	uint8_t endcaps = player->get_buildcaps(end);

	// we should not cross fields with road or flags (or any other immovable)
	if ((map.get_immovable(start)) && !(id == CheckStep::stepFirst)) {
		return false;
	}

	// Calculate cost and passability
	if (!(endcaps & movecaps))
		return false;

	// Check for blocking immovables
	if (BaseImmovable const* const imm = map.get_immovable(end))
		if (imm->get_size() >= BaseImmovable::SMALL) {
			if (id != CheckStep::stepLast && !open_end)
				return false;

			if (dynamic_cast<Flag const*>(imm))
				return true;

			if (!dynamic_cast<Road const*>(imm) || !(endcaps & BUILDCAPS_FLAG))
				return false;
		}

	return true;
}

bool CheckStepRoadAI::reachable_dest(const Map& map, const FCoords& dest) const {
	NodeCaps const caps = dest.field->nodecaps();

	if (!(caps & movecaps)) {
		if (!((movecaps & MOVECAPS_SWIM) && (caps & MOVECAPS_WALK)))
			return false;

		if (!map.can_reach_by_water(dest))
			return false;
	}

	return true;
}

// We are looking for fields we can walk on
// and owned by hostile player.
FindNodeEnemy::FindNodeEnemy(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeEnemy::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & MOVECAPS_WALK) && fc.field->get_owned_by() != 0 &&
	       player->is_hostile(*game.get_player(fc.field->get_owned_by()));
}

// We are looking for buildings owned by hostile player
// (sometimes there is a enemy's teritorry without buildings, and
// this confuses the AI)
FindNodeEnemiesBuilding::FindNodeEnemiesBuilding(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeEnemiesBuilding::accept(const Map&, const FCoords& fc) const {
	return (fc.field->get_immovable()) && fc.field->get_owned_by() != 0 &&
	       player->is_hostile(*game.get_player(fc.field->get_owned_by()));
}

// When looking for unowned terrain to acquire, we are actually
// only interested in fields we can walk on.
// Fields should either be completely unowned or owned by an opposing player
FindNodeUnowned::FindNodeUnowned(Player* p, Game& g, bool oe)
   : player(p), game(g), only_enemies(oe) {
}

bool FindNodeUnowned::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & MOVECAPS_WALK) &&
	       ((fc.field->get_owned_by() == 0) ||
	        player->is_hostile(*game.get_player(fc.field->get_owned_by()))) &&
	       (!only_enemies || (fc.field->get_owned_by() != 0));
}

// Sometimes we need to know how many nodes our allies owns
FindNodeAllyOwned::FindNodeAllyOwned(Player* p, Game& g, PlayerNumber n)
   : player(p), game(g), player_number(n) {
}

bool FindNodeAllyOwned::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & MOVECAPS_WALK) && (fc.field->get_owned_by() != 0) &&
	       (fc.field->get_owned_by() != player_number) &&
	       !player->is_hostile(*game.get_player(fc.field->get_owned_by()));
}

// When looking for unowned terrain to acquire, we must
// pay speciall attention to fields where mines can be built.
// Fields should be completely unowned
FindNodeUnownedMineable::FindNodeUnownedMineable(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeUnownedMineable::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & BUILDCAPS_MINE) && (fc.field->get_owned_by() == 0);
}

FindNodeUnownedBuildable::FindNodeUnownedBuildable(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeUnownedBuildable::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & BUILDCAPS_SIZEMASK) && (fc.field->get_owned_by() == 0);
}

// Unowned but walkable fields nearby
FindNodeUnownedWalkable::FindNodeUnownedWalkable(Player* p, Game& g) : player(p), game(g) {
}

bool FindNodeUnownedWalkable::accept(const Map&, const FCoords& fc) const {
	return (fc.field->nodecaps() & MOVECAPS_WALK) && (fc.field->get_owned_by() == 0);
}

// Looking only for mines-capable fields nearby
// of specific type
FindNodeMineable::FindNodeMineable(Game& g, DescriptionIndex r) : game(g), res(r) {
}

bool FindNodeMineable::accept(const Map&, const FCoords& fc) const {

	return (fc.field->nodecaps() & BUILDCAPS_MINE) && (fc.field->get_resources() == res);
}

// Fishers and fishbreeders must be built near water
FindNodeWater::FindNodeWater(const World& world) : world_(world) {
}

bool FindNodeWater::accept(const Map& map, const FCoords& coord) const {
	return (world_.terrain_descr(coord.field->terrain_d()).get_is() &
	        TerrainDescription::Is::kWater) ||
	       (world_.terrain_descr(map.get_neighbour(coord, WALK_W).field->terrain_r()).get_is() &
	        TerrainDescription::Is::kWater) ||
	       (world_.terrain_descr(map.get_neighbour(coord, WALK_NW).field->terrain_r()).get_is() &
	        TerrainDescription::Is::kWater);
}

bool FindNodeOpenWater::accept(const Map& /* map */, const FCoords& coord) const {
	return !(coord.field->nodecaps() & MOVECAPS_WALK) && (coord.field->nodecaps() & MOVECAPS_SWIM);
}

// FindNodeWithFlagOrRoad
bool FindNodeWithFlagOrRoad::accept(const Map&, FCoords fc) const {
	if (upcast(PlayerImmovable const, pimm, fc.field->get_immovable()))
		return (dynamic_cast<Flag const*>(pimm) ||
		        (dynamic_cast<Road const*>(pimm) && (fc.field->nodecaps() & BUILDCAPS_FLAG)));
	return false;
}

NearFlag::NearFlag(const Flag& f, int32_t const c, int32_t const d)
   : flag(&f), cost(c), distance(d) {
}

BuildableField::BuildableField(const Widelands::FCoords& fc)
	: coords(fc),
	  field_info_expiration(20000),
	  preferred(false),
	  enemy_nearby(0),
	  enemy_accessible_(false),
	  unowned_land_nearby(0),
	  near_border(false),
	  unowned_mines_spots_nearby(0),
	  trees_nearby(0),
	  // explanation of starting values
	  // this is done to save some work for AI (CPU utilization)
	  // base rules are:
	  // count of rocks can only decrease, so  amount of rocks
	  // is recalculated only when previous count is positive
	  // count of water fields are stable, so if the current count is
	  // non-negative, water is not recaldulated
	  rocks_nearby(1),
	  water_nearby(-1),
	  open_water_nearby(-1),
	  distant_water(0),
	  fish_nearby(-1),
	  critters_nearby(-1),
	  ground_water(1),
	  space_consumers_nearby(0),
	  rangers_nearby(0),
	  area_military_capacity(0),
	  military_loneliness(1000),
	  military_in_constr_nearby(0),
	  own_military_presence(0),
	  enemy_military_presence(0),
	  ally_military_presence(0),
	  military_stationed(0),
	  unconnected_nearby(false),
	  military_unstationed(0),
	  is_portspace(Widelands::ExtendedBool::kUnset),
	  port_nearby(false),
	  portspace_nearby(Widelands::ExtendedBool::kUnset),
	  max_buildcap_nearby(0),
	  last_resources_check_time(0),
	  military_score_(0),
	  inland(false),
	  local_soldier_capacity(0),
      is_militarysite(false) {
}

int32_t BuildableField::own_military_sites_nearby_() {
	return military_stationed + military_unstationed;
}

MineableField::MineableField(const Widelands::FCoords& fc)
   : coords(fc),
     field_info_expiration(20000),
     preferred(false),
     mines_nearby(0),
     same_mine_fields_nearby(0) {
}

EconomyObserver::EconomyObserver(Widelands::Economy& e) : economy(e) {
	dismantle_grace_time = std::numeric_limits<int32_t>::max();
}

int32_t BuildingObserver::total_count() const {
	return cnt_built + cnt_under_construction;
}

Widelands::AiModeBuildings BuildingObserver::aimode_limit_status() {
	if (total_count() > cnt_limit_by_aimode) {
		return Widelands::AiModeBuildings::kLimitExceeded;
	} else if (total_count() == cnt_limit_by_aimode) {
		return Widelands::AiModeBuildings::kOnLimit;
	} else {
		return Widelands::AiModeBuildings::kAnotherAllowed;
	}
}
bool BuildingObserver::buildable(Widelands::Player& p) {
	return is_buildable && p.is_building_type_allowed(id);
}

// Computer player does not get notification messages about enemy militarysites
// and warehouses, so following is collected based on observation
// It is conventient to have some information preserved, like nearby minefields,
// when it was attacked, whether it is warehouse and so on
// Also AI test more such targets when considering attack and calculated score is
// is stored in the observer
EnemySiteObserver::EnemySiteObserver()
   : is_warehouse(false),
     attack_soldiers_strength(0),
     defenders_strength(0),
     stationed_soldiers(0),
     last_time_attackable(std::numeric_limits<uint32_t>::max()),
     last_tested(0),
     score(0),
     mines_nearby(Widelands::ExtendedBool::kUnset),
     no_attack_counter(0) {
}

// as all mines have 3 levels, AI does not know total count of mines per mined material
// so this observer will be used for this
MineTypesObserver::MineTypesObserver() : in_construction(0), finished(0) {
}

ManagementData::ManagementData() {
		scores[0] = 1;
		scores[1] = 1;
		scores[2] = 1;
		review_count = 0;
		last_mutate_time = 0;
		next_neuron_id = 0;
		performance_change = 0;
		mutation_multiplicator = 1;
}

Neuron::Neuron(int8_t w, uint8_t f, uint16_t i) : 
	weight(w),type(f), id(i) {
	assert(type < neuron_curves.size());
	assert(weight >= -100 && weight <= 100);
	lowest_pos = std::numeric_limits<uint8_t>::max();
	highest_pos = std::numeric_limits<uint8_t>::min();
	recalculate();
}


FNeuron::FNeuron(uint32_t c){
	core = c;
}

bool FNeuron::get_result(const bool bool1, const bool bool2, const bool bool3, const bool bool4, const bool bool5){
	//printf ("returning %lu (pos: %2d, integer representation %8u)\n",
		//core.test(bool1 * 16 + bool2 * 8 + bool3 * 4 + bool4 * 2 + bool5),
		//bool1 * 16 + bool2 * 8 + bool3 * 4 + bool4 * 2 + bool5,
		//core.to_ulong()
		//);
	return core.test(bool1 * 16 + bool2 * 8 + bool3 * 4 + bool4 * 2 + bool5);
}

uint32_t FNeuron::get_int(){
	return core.to_ulong();
}

void FNeuron::flip_bit(const uint8_t pos){
	assert (pos < f_neuron_bit_size);
	core.flip(pos);
}

void Neuron::set_weight(int8_t w) {
	if (w > 100) {
		weight = 100;
	}else if (w < -100) {
		weight = -100;
	} else {
		weight = w;
	}
}


void Neuron::recalculate() {
	//printf (" type: %d\n", type);
	assert (neuron_curves.size() > type);
	for (int8_t i = 0; i <= 20; i+=1) {
		results[i] = weight * neuron_curves[type][i] / 100;
		//printf (" neuron output: %3d\n", results[i]);
	}
}

int8_t Neuron::get_result(const uint8_t pos){
	assert(pos <= 20);
	return results[pos];
}

int8_t Neuron::get_result_safe(int32_t pos, const bool absolute){
	if (pos > highest_pos) {highest_pos = pos;};
	if (pos < lowest_pos) {lowest_pos = pos;};
	if (pos < 0) {
		pos = 0;
	}
	if (pos > 20) {
		pos = 20;
	}
	assert(pos <= 20);
	assert (results[pos] >= -100 && results[pos] <=100);
	if (absolute) {
		return std::abs(results[pos]);
	}	
	return results[pos];
}

void Neuron::set_type(uint8_t new_type) {
	type = new_type;
}

// this randomly sets new values into neurons and AI magic numbers
void ManagementData::mutate(const uint32_t gametime) {

	int16_t probability = -1;

	probability = get_military_number_at(13) / 8 + 15;
	
	probability *= mutation_multiplicator;
	probability /= 10;
	assert(probability > 0);
	
	printf ("    ... mutating , time since last mutation: %6d, probability: 1 / %d (multiplicator :%d)\n",
	(gametime - last_mutate_time) / 1000 / 60,
	probability,
	mutation_multiplicator);
	last_mutate_time = gametime;

	for (uint16_t i = 0; i < magic_numbers_size; i += 1){
	   	if (std::rand() % probability == 0) {
			// poor man's gausian distribution probability
			int16_t new_value = ((-100 + std::rand() % 200) * 3 -100 + std::rand() % 200) / 4;
			assert (new_value >= -100 && new_value <=100);
			set_military_number_at(i,new_value);
			printf ("      Magic number %d: new value: %4d\n", i, new_value);
		} 
	}

	// Modifying pool of neurons	
	for (auto& item : neuron_pool){
		if (std::rand() % probability / 2 == 0) { //exemption
			item.set_weight(((-100 + std::rand() % 200) * 3 -100 + std::rand() % 200) / 4);
			pd->neuron_weights[item.get_id()] = item.get_weight();
			assert(neuron_curves.size() > 0);
			item.set_type(std::rand() % neuron_curves.size());
			pd->neuron_functs[item.get_id()] = item.get_type();
			printf ("      Neuron %2d: new weight: %4d, new curve: %d\n", item.get_id(), item.get_weight(), item.get_type());
			item.recalculate();
		} 
	}

	// Modifying pool of f-neurons
	uint16_t pos = 0;	
	for (auto& item : f_neuron_pool){
		bool fneuron_changed = false;
		for (uint8_t i = 0; i < f_neuron_bit_size; i += 1) {
			if (std::rand() % probability == 0) {
				item.flip_bit(i);
				fneuron_changed = true;
			}
		}

		if (fneuron_changed) {
			pd->f_neurons[pos] = item.get_int();
			printf ("      F-Neuron %2d: new value: %ul\n", pos, item.get_int());
		}
		pos += 1; 
	}

	test_consistency();
	dump_data();
}


void ManagementData::review(const uint32_t gametime, PlayerNumber pn,
 const uint32_t strength, const uint32_t strength_delta, const uint32_t land, const uint32_t land_delta, const bool is_attacker) {
	assert(!pd->magic_numbers.empty());
	scores[0] = scores[1];
	scores[1] = scores[2];
	scores[2] = land / 3 + static_cast<int32_t>(land_delta) / 6 + static_cast<int32_t>(strength_delta) - 100 + 100 * is_attacker;
	assert (scores[2] > -10000 && scores[2] < 100000);
	
	printf (" %d %s: reviewing AI mngm. data, score: %4d -> %4d -> %4d (land:%3d, delta: %3d, strength: %3d, delta:%3d, attacker: %s)\n",
	pn, gamestring_with_leading_zeros(gametime), scores[0], scores[1], scores[2],
	land, land_delta, strength, strength_delta, (is_attacker)?"YES":"NO");

	//If under 25 seconds we re-initialize
	if (gametime < 25 * 1000){
		printf (" %d - reinitializing DNA\n", pn);
		initialize(pn, true);
			mutate(gametime);
		dump_data();
	} else {
		printf ("   still using mutate from %d minutes ago:\n",
		(gametime - last_mutate_time) / 1000 / 60); 
		dump_data();
	}
	
	review_count += 1;	
}

void ManagementData::initialize( const uint8_t pn, const bool reinitializing) {
	printf (" ... initialize starts %s\n", reinitializing?" * reinitializing *":"");


    //AutoSCore_AIDNA_1
    const std::vector<int16_t> AI_initial_military_numbers_A =
      { -10 ,  -43 ,  -62 ,   19 ,  -31 ,   53 ,  -24 ,   61 ,   -9 ,  -54 ,  //AutoContent_01_AIDNA_1
         -9 ,  -32 ,   33 ,   14 ,   40 ,  -51 ,   26 ,   51 ,   22 ,   39 ,  //AutoContent_02_AIDNA_1
         26 ,   11 ,  -11 ,   34 ,  -52 ,  -15 ,   81 ,  -75 ,   81 ,  -57 ,  //AutoContent_03_AIDNA_1
         58 ,   33 ,  -49 ,  -35 ,  -54 ,    2 ,   15 ,   91 ,   19 ,   50 ,  //AutoContent_04_AIDNA_1
         23 ,   35 ,   40 ,  -28 ,  -65 ,  -20 ,  -52 ,  -65 ,   38 ,  -23 ,  //AutoContent_05_AIDNA_1
          0 ,   46 ,  -42 ,  -19 ,  -56 ,   -2 ,   39 ,    1 ,   -2 ,   92 ,  //AutoContent_06_AIDNA_1
        -31 ,   75 ,  -24 ,   46 ,  -53 ,  -48 ,   27 ,  -56 ,   11 ,    0 ,  //AutoContent_07_AIDNA_1
        -24 ,  -70 ,   40 ,  -28 ,  -33 ,   46 ,   43 ,  -48 ,  -75 ,   43 ,  //AutoContent_08_AIDNA_1
        -11 ,  -37 ,  -49 ,  -16 ,   59 ,    2 ,  -55 ,   -7 ,   33 ,  -28 ,  //AutoContent_09_AIDNA_1
        -55 ,   77 ,    1 ,   53 ,   44 ,   23 ,   59 ,  -38 ,  -25 ,   33  //AutoContent_10_AIDNA_1
       }
		;
	
	assert(magic_numbers_size == AI_initial_military_numbers_A.size());
	
	const std::vector<int8_t> input_weights_A =
      {   38 ,   39 ,  -48 ,   64 ,   10 ,   22 ,   68 ,   51 ,  -69 ,   74 ,  //AutoContent_11_AIDNA_1
         47 ,   74 ,  -62 ,  -56 ,   27 ,  -68 ,   -9 ,   53 ,  -41 ,   27 ,  //AutoContent_12_AIDNA_1
        -81 ,  -17 ,  -17 ,  -89 ,   47 ,  -38 ,    1 ,   49 ,  -34 ,  -36 ,  //AutoContent_13_AIDNA_1
         39 ,  -13 ,   83 ,   14 ,    0 ,  -41 ,   25 ,   35 ,   38 ,   15 ,  //AutoContent_14_AIDNA_1
        -37 ,   70 ,   40 ,   66 ,   57 ,   -2 ,  -10 ,    4 ,   -1 ,  -23 ,  //AutoContent_15_AIDNA_1
        -15 ,   75 ,   -5 ,  -34 ,    2 ,   11 ,  -72 ,  -34 ,  -63 ,  -46 ,  //AutoContent_16_AIDNA_1
         45 ,   71 ,  -22 ,  -30 ,   73 ,  -80 ,    6 ,  -31 ,  -44 ,   -8 ,  //AutoContent_17_AIDNA_1
         90 ,   18 ,   51 ,   25 ,   20 ,   -1 ,    0 ,   41 ,  -48 ,   54  //AutoContent_18_AIDNA_1
	}
			;
	const std::vector<int8_t> input_func_A =
      {    2 ,    0 ,    1 ,    2 ,    1 ,    0 ,    1 ,    1 ,    0 ,    1 ,  //AutoContent_19_AIDNA_1
          0 ,    1 ,    1 ,    0 ,    2 ,    0 ,    2 ,    0 ,    2 ,    2 ,  //AutoContent_20_AIDNA_1
          2 ,    1 ,    2 ,    0 ,    2 ,    0 ,    0 ,    2 ,    2 ,    1 ,  //AutoContent_21_AIDNA_1
          2 ,    2 ,    2 ,    0 ,    2 ,    0 ,    1 ,    1 ,    1 ,    2 ,  //AutoContent_22_AIDNA_1
          1 ,    0 ,    0 ,    0 ,    2 ,    0 ,    0 ,    0 ,    2 ,    1 ,  //AutoContent_23_AIDNA_1
          1 ,    1 ,    2 ,    1 ,    2 ,    2 ,    2 ,    2 ,    2 ,    0 ,  //AutoContent_24_AIDNA_1
          1 ,    2 ,    1 ,    2 ,    2 ,    0 ,    1 ,    1 ,    0 ,    1 ,  //AutoContent_25_AIDNA_1
          2 ,    0 ,    0 ,    2 ,    2 ,    2 ,    0 ,    0 ,    1 ,    2  //AutoContent_26_AIDNA_1
	}
		;
	assert(neuron_pool_size == input_func_A.size());
	assert(neuron_pool_size == input_weights_A.size());

	const std::vector<uint32_t> f_neurons_A =
      {  2275058077 ,  2349013783 ,  3250045604 ,  3492580850 ,  2886322614 ,  3449182538 ,  4209933435 ,  2981002627 ,  1279230583 ,  644140296 ,  //AutoContent_27_AIDNA_1
        2539283239 ,  3716361952 ,  1378002920 ,  3755996089 ,  2392358607 ,  3774533734 ,  2200565728 ,  4212751805 ,  3200141340 ,  2452593430 ,  //AutoContent_28_AIDNA_1
        2000968742 ,  1619109330 ,  1733139289 ,  62001385 ,  3687680030 ,  2573553513 ,  2975359355 ,  2241118174 ,  2871896954 ,  817940117 ,  //AutoContent_29_AIDNA_1
        2907591077 ,  2921523667 ,  4249473918 ,  4034931778 ,  2672149970 ,  127060486 ,  142337970 ,  1781265881 ,  2525104802 ,  2601443495  //AutoContent_30_AIDNA_1
	 };
	assert(f_neuron_pool_size == f_neurons_A.size());

		
    //AutoSCore_AIDNA_2
	const std::vector<int16_t> AI_initial_military_numbers_B =
      { -10 ,  -43 ,  -54 ,   19 ,   17 ,   53 ,   35 ,   61 ,   -9 ,  -54 ,  //AutoContent_01_AIDNA_2
         -9 ,  -32 ,   33 ,   14 ,   40 ,  -51 ,   26 ,   51 ,   22 ,  -51 ,  //AutoContent_02_AIDNA_2
          6 ,   11 ,  -11 ,   34 ,  -52 ,  -15 ,   15 ,   52 ,   81 ,   12 ,  //AutoContent_03_AIDNA_2
         58 ,    1 ,  -49 ,  -35 ,  -54 ,    2 ,  -61 ,   91 ,   75 ,   10 ,  //AutoContent_04_AIDNA_2
         23 ,   -4 ,   40 ,  -28 ,   39 ,  -20 ,  -52 ,  -65 ,   38 ,  -23 ,  //AutoContent_05_AIDNA_2
          0 ,   46 ,  -42 ,  -19 ,  -56 ,   -2 ,   39 ,    1 ,   37 ,   92 ,  //AutoContent_06_AIDNA_2
        -31 ,   75 ,  -14 ,   46 ,  -53 ,  -48 ,   62 ,  -56 ,   11 ,    0 ,  //AutoContent_07_AIDNA_2
        -24 ,  -70 ,   40 ,   -7 ,  -76 ,   46 ,   43 ,  -48 ,  -75 ,   43 ,  //AutoContent_08_AIDNA_2
        -11 ,  -37 ,  -42 ,  -16 ,   59 ,    2 ,  -55 ,   -7 ,   33 ,  -28 ,  //AutoContent_09_AIDNA_2
        -55 ,   77 ,    1 ,  -34 ,   49 ,   23 ,   59 ,  -38 ,  -25 ,   33  //AutoContent_10_AIDNA_2
		}
		;
	assert(magic_numbers_size == AI_initial_military_numbers_B.size());
		
	const std::vector<int8_t> input_weights_B =
      {   38 ,   39 ,  -40 ,   64 ,   10 ,   22 ,   11 ,   52 ,  -69 ,   20 ,  //AutoContent_11_AIDNA_2
         47 ,   74 ,  -46 ,  -56 ,   50 ,  -15 ,   -9 ,   53 ,  -41 ,   27 ,  //AutoContent_12_AIDNA_2
        -81 ,  -17 ,  -81 ,  -89 ,   47 ,   74 ,  -13 ,   49 ,  -48 ,  -36 ,  //AutoContent_13_AIDNA_2
         39 ,   42 ,   83 ,   14 ,  -60 ,   46 ,   25 ,  -39 ,  -29 ,   15 ,  //AutoContent_14_AIDNA_2
        -37 ,   70 ,   40 ,   66 ,  -42 ,   -6 ,  -10 ,   38 ,   -1 ,   32 ,  //AutoContent_15_AIDNA_2
        -33 ,   75 ,   -5 ,   25 ,    2 ,   11 ,  -72 ,  -34 ,  -63 ,   26 ,  //AutoContent_16_AIDNA_2
         45 ,  -30 ,   67 ,  -30 ,   29 ,  -80 ,  -50 ,  -31 ,   52 ,   39 ,  //AutoContent_17_AIDNA_2
        -41 ,   18 ,   37 ,   74 ,   17 ,   -1 ,    0 ,   41 ,  -10 ,   54  //AutoContent_18_AIDNA_2
}
	      ;
	
	const std::vector<int8_t> input_func_B = 
      {    2 ,    0 ,    0 ,    2 ,    1 ,    0 ,    2 ,    0 ,    0 ,    0 ,  //AutoContent_19_AIDNA_2
          0 ,    1 ,    0 ,    0 ,    2 ,    2 ,    2 ,    0 ,    2 ,    2 ,  //AutoContent_20_AIDNA_2
          2 ,    1 ,    2 ,    0 ,    2 ,    1 ,    0 ,    2 ,    2 ,    1 ,  //AutoContent_21_AIDNA_2
          2 ,    2 ,    2 ,    0 ,    1 ,    1 ,    1 ,    1 ,    1 ,    0 ,  //AutoContent_22_AIDNA_2
          1 ,    0 ,    0 ,    0 ,    0 ,    0 ,    0 ,    0 ,    2 ,    2 ,  //AutoContent_23_AIDNA_2
          2 ,    1 ,    2 ,    0 ,    2 ,    2 ,    2 ,    2 ,    2 ,    2 ,  //AutoContent_24_AIDNA_2
          1 ,    1 ,    0 ,    2 ,    2 ,    0 ,    2 ,    1 ,    1 ,    2 ,  //AutoContent_25_AIDNA_2
          2 ,    0 ,    1 ,    0 ,    1 ,    2 ,    0 ,    0 ,    2 ,    2  //AutoContent_26_AIDNA_2
}
		;
		assert(neuron_pool_size == input_func_B.size());
		assert(neuron_pool_size == input_weights_B.size());

      
	const std::vector<uint32_t> f_neurons_B =
      {  62562713 ,  2374179615 ,  2239355300 ,  1576541290 ,  3206244534 ,  4011284840 ,  980384989 ,  1500277959 ,  3287257335 ,  878986726 ,  //AutoContent_27_AIDNA_2
        3817564959 ,  3716558496 ,  1511172074 ,  1474288560 ,  244901583 ,  2707089522 ,  2208954080 ,  4077410744 ,  2529056793 ,  2150480070 ,  //AutoContent_28_AIDNA_2
        1982056650 ,  1690412098 ,  621805389 ,  2644068899 ,  3404646426 ,  358969193 ,  3211268537 ,  2912808398 ,  2853153608 ,  806409745 ,  //AutoContent_29_AIDNA_2
        2974765477 ,  2901324147 ,  4249710942 ,  4035001574 ,  2506474962 ,  127322639 ,  159048626 ,  3950704091 ,  2519128811 ,  2668295399  //AutoContent_30_AIDNA_2
	 };
	assert(f_neuron_pool_size == f_neurons_B.size());


    //AutoSCore_AIDNA_3
	const std::vector<int16_t> AI_initial_military_numbers_C =
      { -10 ,  -43 ,  -54 ,   19 ,   17 ,   53 ,   35 ,   61 ,   -9 ,  -54 ,  //AutoContent_01_AIDNA_3
         -9 ,  -32 ,   33 ,   24 ,   40 ,  -51 ,   26 ,   51 ,   22 ,  -51 ,  //AutoContent_02_AIDNA_3
         26 ,   42 ,  -11 ,   34 ,  -52 ,  -15 ,   15 ,   72 ,   81 ,   12 ,  //AutoContent_03_AIDNA_3
         58 ,   33 ,  -49 ,  -35 ,  -54 ,  -46 ,   28 ,   91 ,   19 ,   10 ,  //AutoContent_04_AIDNA_3
         23 ,   35 ,   40 ,  -28 ,   39 ,  -20 ,  -52 ,  -65 ,   38 ,  -23 ,  //AutoContent_05_AIDNA_3
          0 ,   46 ,  -42 ,  -19 ,  -56 ,   -2 ,   39 ,    1 ,   -2 ,   92 ,  //AutoContent_06_AIDNA_3
        -31 ,   75 ,  -14 ,   46 ,  -53 ,  -48 ,   62 ,  -56 ,   11 ,    0 ,  //AutoContent_07_AIDNA_3
        -24 ,  -70 ,  -41 ,  -28 ,  -33 ,   46 ,   43 ,  -48 ,   38 ,   43 ,  //AutoContent_08_AIDNA_3
        -11 ,  -37 ,  -42 ,  -16 ,   59 ,    2 ,  -55 ,   -7 ,   33 ,  -40 ,  //AutoContent_09_AIDNA_3
        -65 ,   77 ,    1 ,   53 ,   60 ,   23 ,   59 ,  -38 ,  -25 ,   33  //AutoContent_10_AIDNA_3
       }

		;
	
		assert(magic_numbers_size == AI_initial_military_numbers_C.size());
	
	const std::vector<int8_t> input_weights_C =
      {   38 ,   39 ,  -46 ,   64 ,   10 ,   22 ,   11 ,   52 ,  -69 ,   20 ,  //AutoContent_11_AIDNA_3
         47 ,   74 ,  -46 ,  -56 ,   50 ,  -15 ,   -9 ,   53 ,  -49 ,   27 ,  //AutoContent_12_AIDNA_3
        -81 ,  -17 ,   30 ,  -89 ,   34 ,  -38 ,  -13 ,   49 ,  -48 ,  -36 ,  //AutoContent_13_AIDNA_3
         39 ,  -13 ,   83 ,   14 ,    0 ,   46 ,   25 ,  -67 ,  -29 ,  -25 ,  //AutoContent_14_AIDNA_3
         12 ,  -76 ,   40 ,   66 ,  -42 ,   -6 ,  -10 ,   38 ,   -1 ,   32 ,  //AutoContent_15_AIDNA_3
        -15 ,   75 ,   -5 ,  -69 ,    2 ,   11 ,  -72 ,  -34 ,  -63 ,  -46 ,  //AutoContent_16_AIDNA_3
         45 ,  -30 ,  -22 ,  -30 ,   29 ,  -80 ,  -50 ,   66 ,   52 ,   64 ,  //AutoContent_17_AIDNA_3
         82 ,   18 ,   24 ,   74 ,  -59 ,   -1 ,    0 ,   41 ,  -10 ,   54  //AutoContent_18_AIDNA_3
       }
			;
	const std::vector<int8_t> input_func_C =
      {    2 ,    0 ,    2 ,    2 ,    1 ,    0 ,    2 ,    0 ,    0 ,    0 ,  //AutoContent_19_AIDNA_3
          0 ,    1 ,    0 ,    0 ,    2 ,    2 ,    2 ,    0 ,    1 ,    2 ,  //AutoContent_20_AIDNA_3
          2 ,    1 ,    2 ,    0 ,    0 ,    0 ,    0 ,    2 ,    2 ,    1 ,  //AutoContent_21_AIDNA_3
          2 ,    2 ,    2 ,    0 ,    2 ,    1 ,    1 ,    0 ,    1 ,    1 ,  //AutoContent_22_AIDNA_3
          1 ,    2 ,    0 ,    0 ,    0 ,    0 ,    0 ,    0 ,    2 ,    2 ,  //AutoContent_23_AIDNA_3
          1 ,    1 ,    2 ,    2 ,    2 ,    2 ,    2 ,    2 ,    2 ,    0 ,  //AutoContent_24_AIDNA_3
          1 ,    1 ,    1 ,    2 ,    2 ,    0 ,    2 ,    0 ,    1 ,    0 ,  //AutoContent_25_AIDNA_3
          0 ,    0 ,    2 ,    0 ,    0 ,    2 ,    0 ,    0 ,    2 ,    2  //AutoContent_26_AIDNA_3
       }
			;
	assert(neuron_pool_size == input_func_C.size());
	assert(neuron_pool_size == input_weights_C.size());
	
	const std::vector<uint32_t> f_neurons_C =
      {  62581145 ,  2374703887 ,  2239355053 ,  1576605802 ,  4239362998 ,  3383908618 ,  4184767579 ,  562986183 ,  1194164823 ,  3160688582 ,  //AutoContent_27_AIDNA_3
        3817548575 ,  2642882208 ,  1242736618 ,  2011160240 ,  211414735 ,  2689261682 ,  61470432 ,  3812170173 ,  2493405209 ,  2454657943 ,  //AutoContent_28_AIDNA_3
        1982299298 ,  1686211906 ,  630193997 ,  2644331171 ,  3403384846 ,  358952809 ,  827375931 ,  2777673678 ,  2853153608 ,  549643797 ,  //AutoContent_29_AIDNA_3
        2957988261 ,  2753621459 ,  4249743836 ,  4035001574 ,  2540029394 ,  129157647 ,  159048626 ,  3950699995 ,  2527485675 ,  2668556519  //AutoContent_30_AIDNA_3
	 };
	assert(f_neuron_pool_size == f_neurons_C.size());

		
    //AutoSCore_AIDNA_4
	const std::vector<int16_t> AI_initial_military_numbers_D =
      { -10 ,  -43 ,  -62 ,   19 ,   17 ,   53 ,  -24 ,   61 ,   -9 ,  -54 ,  //AutoContent_01_AIDNA_4
         -9 ,  -32 ,   33 ,   14 ,   40 ,  -51 ,   26 ,   51 ,   22 ,   39 ,  //AutoContent_02_AIDNA_4
         26 ,   11 ,  -11 ,   34 ,  -52 ,  -15 ,   15 ,  -75 ,   81 ,   12 ,  //AutoContent_03_AIDNA_4
         58 ,   33 ,  -49 ,  -35 ,  -54 ,    2 ,  -61 ,   91 ,   19 ,   10 ,  //AutoContent_04_AIDNA_4
         23 ,   35 ,   40 ,  -28 ,   39 ,  -20 ,  -52 ,  -65 ,   38 ,  -23 ,  //AutoContent_05_AIDNA_4
          0 ,   46 ,  -42 ,  -19 ,  -56 ,   -2 ,   39 ,    1 ,   -2 ,   92 ,  //AutoContent_06_AIDNA_4
        -31 ,   75 ,  -14 ,   46 ,  -53 ,  -48 ,   62 ,   20 ,   11 ,    0 ,  //AutoContent_07_AIDNA_4
        -24 ,  -70 ,   26 ,   -7 ,  -33 ,   46 ,   43 ,  -48 ,   38 ,   43 ,  //AutoContent_08_AIDNA_4
        -11 ,  -37 ,  -42 ,  -16 ,   48 ,    2 ,  -55 ,   -7 ,   33 ,  -28 ,  //AutoContent_09_AIDNA_4
        -55 ,   77 ,    1 ,  -34 ,   49 ,   23 ,   59 ,  -38 ,  -25 ,   33  //AutoContent_10_AIDNA_4
	}
		;
	assert(magic_numbers_size == AI_initial_military_numbers_D.size());
		
	const std::vector<int8_t> input_weights_D =
      {   38 ,   39 ,  -40 ,   64 ,   10 ,   22 ,   11 ,   52 ,  -69 ,   20 ,  //AutoContent_11_AIDNA_4
         47 ,   74 ,   34 ,  -56 ,   50 ,  -15 ,   -9 ,   53 ,  -49 ,   27 ,  //AutoContent_12_AIDNA_4
        -81 ,  -17 ,   13 ,  -89 ,   34 ,   74 ,  -13 ,   49 ,  -48 ,  -36 ,  //AutoContent_13_AIDNA_4
         39 ,  -13 ,   83 ,   14 ,    0 ,   46 ,   25 ,  -39 ,  -29 ,  -25 ,  //AutoContent_14_AIDNA_4
         12 ,   70 ,   40 ,   66 ,  -42 ,   -6 ,  -10 ,   38 ,   -1 ,  -23 ,  //AutoContent_15_AIDNA_4
        -33 ,   75 ,   -5 ,   25 ,    2 ,   11 ,  -72 ,  -34 ,  -63 ,  -46 ,  //AutoContent_16_AIDNA_4
         45 ,  -30 ,   67 ,  -30 ,   29 ,  -80 ,  -50 ,   66 ,   52 ,   -8 ,  //AutoContent_17_AIDNA_4
         82 ,   18 ,   51 ,   74 ,   20 ,   -1 ,    0 ,   41 ,  -61 ,   54  //AutoContent_18_AIDNA_4
	}
	      ;
	
	const std::vector<int8_t> input_func_D = 
      {    2 ,    0 ,    0 ,    2 ,    1 ,    0 ,    2 ,    0 ,    0 ,    0 ,  //AutoContent_19_AIDNA_4
          0 ,    1 ,    0 ,    0 ,    2 ,    2 ,    2 ,    0 ,    1 ,    2 ,  //AutoContent_20_AIDNA_4
          2 ,    1 ,    2 ,    0 ,    0 ,    1 ,    0 ,    2 ,    2 ,    1 ,  //AutoContent_21_AIDNA_4
          2 ,    2 ,    2 ,    0 ,    2 ,    1 ,    1 ,    1 ,    1 ,    1 ,  //AutoContent_22_AIDNA_4
          1 ,    0 ,    0 ,    0 ,    0 ,    0 ,    0 ,    0 ,    2 ,    1 ,  //AutoContent_23_AIDNA_4
          2 ,    1 ,    2 ,    0 ,    2 ,    2 ,    2 ,    2 ,    2 ,    0 ,  //AutoContent_24_AIDNA_4
          1 ,    1 ,    0 ,    2 ,    2 ,    0 ,    2 ,    0 ,    1 ,    1 ,  //AutoContent_25_AIDNA_4
          0 ,    0 ,    0 ,    0 ,    2 ,    2 ,    0 ,    0 ,    0 ,    2  //AutoContent_26_AIDNA_4
	}
		;
	assert(neuron_pool_size == input_func_D.size());
	assert(neuron_pool_size == input_weights_D.size());

	const std::vector<uint32_t> f_neurons_D =
      {  62562745 ,  495131455 ,  2172246181 ,  1576606826 ,  3851405718 ,  3383908618 ,  3144580219 ,  567180487 ,  1177387639 ,  3026470342 ,  //AutoContent_27_AIDNA_4
        3817548575 ,  3716624096 ,  1385342954 ,  1474255536 ,  244903631 ,  2701846642 ,  2334783460 ,  3812436413 ,  348018713 ,  2454690711 ,  //AutoContent_28_AIDNA_4
        2114419874 ,  3767635266 ,  621789005 ,  2644331139 ,  3405678618 ,  358985577 ,  3144159673 ,  2777542606 ,  2853022586 ,  943871573 ,  //AutoContent_29_AIDNA_4
        3108983205 ,  2753620435 ,  4258132318 ,  4037028928 ,  2540029392 ,  127060495 ,  190505906 ,  4017812953 ,  3064389355 ,  2601443559  //AutoContent_30_AIDNA_4
	 };
	assert(f_neuron_pool_size == f_neurons_D.size());


	for (uint16_t i =  0; i < magic_numbers_size; i = i+1){
		if (std::abs(AI_initial_military_numbers_A[i]) < 45 &&
		std::abs(AI_initial_military_numbers_B[i]) < 45 &&
		std::abs(AI_initial_military_numbers_C[i]) < 45 &&
		std::abs(AI_initial_military_numbers_D[i]) < 45 ) {
			printf ("military number rebalance candidate: %2d: %4d %4d %4d %4d\n",
			i,
			AI_initial_military_numbers_A[i],
			AI_initial_military_numbers_B[i],
			AI_initial_military_numbers_C[i],
			AI_initial_military_numbers_D[i] );
		}
	}
	
	for (uint16_t i =  0; i < neuron_pool_size; i = i+1){
		uint8_t count = 0;
		count  += std::abs(input_weights_A[i]) > 80;
		count  += std::abs(input_weights_B[i]) > 80;
		count  += std::abs(input_weights_C[i]) > 80;
		count  += std::abs(input_weights_D[i]) > 80;	
		if (count >= 2 ) {
			printf ("pool weights rebalance candidate: %2d: %4d %4d %4d %4d\n",
			i,
			input_weights_A[i],
			input_weights_B[i],
			input_weights_C[i],
			input_weights_D[i] );
		}
	}

	printf (" %d: initializing AI's DNA\n", pn);

	// filling vector with zeros
	if (!reinitializing) {
		for (uint16_t i =  0; i < magic_numbers_size; i = i+1){
			pd->magic_numbers.push_back(0);
		}
	}
	assert (pd->magic_numbers.size() == magic_numbers_size);
	
	const uint8_t parent = std::rand() % 4;
	const uint8_t parent2 = std::rand() % 4;
	
	printf (" ... DNA initialization (parent: %d, secondary parent: %d)\n", parent, parent2);
	
	for (uint16_t i = 0; i < magic_numbers_size; i += 1){
		// Child inherites DNA with probability 5:1 from main parent
		const uint8_t dna_donor = (std::rand() % 6 > 0) ? parent : parent2;
		
		switch ( dna_donor ) {
			case 0 : 
				set_military_number_at(i,AI_initial_military_numbers_A[i]);
				break;
			case 1 : 
				set_military_number_at(i,AI_initial_military_numbers_B[i]);
				break;
			case 2 : 
				set_military_number_at(i,AI_initial_military_numbers_C[i]);
				break;
			case 3 : 
				set_military_number_at(i,AI_initial_military_numbers_D[i]);
				break;
			default:
				printf ("parent %d?\n", dna_donor);
				NEVER_HERE();
			}
		}

	if (reinitializing) {
		neuron_pool.clear();
		reset_neuron_id();
		pd->neuron_weights.clear();
		pd->neuron_functs.clear();
		f_neuron_pool.clear();
		pd->f_neurons.clear();
	}

	//printf (" ... initialize 2, pool size: %lu\n", neuron_pool.size());
	assert(neuron_pool.empty());
	
	for (uint16_t i = 0; i <neuron_pool_size; i += 1){
		const uint8_t dna_donor = (std::rand() % 6 > 0) ? parent : parent2;
		
		switch ( dna_donor ) {
			case 0 : 
				neuron_pool.push_back(Neuron(input_weights_A[i],input_func_A[i],new_neuron_id()));
				break;
			case 1 : 
				neuron_pool.push_back(Neuron(input_weights_B[i],input_func_B[i],new_neuron_id()));
				break;
			case 2 : 
				neuron_pool.push_back(Neuron(input_weights_C[i],input_func_C[i],new_neuron_id()));
				break;
			case 3 : 
				neuron_pool.push_back(Neuron(input_weights_D[i],input_func_D[i],new_neuron_id()));
				break;
			default:
				printf ("parent %d?\n", dna_donor);
				NEVER_HERE();
		}
	}


	for (uint16_t i = 0; i <f_neuron_pool_size; i += 1){
		const uint8_t dna_donor = (std::rand() % 6 > 0) ? parent : parent2;
		switch ( dna_donor ) {
			case 0 : 
				f_neuron_pool.push_back(FNeuron(f_neurons_A[i]));				
				break;
			case 1 : 
				f_neuron_pool.push_back(FNeuron(f_neurons_B[i]));	
				break;
			case 2 : 
				f_neuron_pool.push_back(FNeuron(f_neurons_C[i]));					
				break;
			case 3 : 
				f_neuron_pool.push_back(FNeuron(f_neurons_D[i]));	
				break;
			default:
				printf ("parent %d?\n", dna_donor);
				NEVER_HERE();
		}
	}
	
	//printf (" ... initialize 2.5, pool size: %lu\n", neuron_pool.size());
	assert(pd->neuron_weights.empty());
	assert(pd->neuron_functs.empty());	
	assert(pd->f_neurons.empty());
		
	for (uint32_t i = 0; i < neuron_pool_size; i = i+1){
		pd->neuron_weights.push_back(neuron_pool[i].get_weight());
		pd->neuron_functs.push_back(neuron_pool[i].get_type());	
	}

	for (uint32_t i = 0; i < f_neuron_pool_size; i = i+1){
		pd->f_neurons.push_back(f_neuron_pool[i].get_int());
	}

	//printf (" ... initialize 3\n");
	
	pd->magic_numbers_size = magic_numbers_size;
	pd->neuron_pool_size = neuron_pool_size;
	pd->f_neuron_pool_size = f_neuron_pool_size;	
	
	test_consistency();
	printf (" %d: DNA initialized\n", pn);
			
}

bool ManagementData::test_consistency() {

	assert (pd->neuron_weights.size() == pd->neuron_pool_size);
	assert (pd->neuron_functs.size() == pd->neuron_pool_size);
	assert (neuron_pool.size() == pd->neuron_pool_size);
	assert (neuron_pool.size() == neuron_pool_size);
	
	assert (pd->magic_numbers_size == magic_numbers_size);			
	assert (pd->magic_numbers.size() == magic_numbers_size);
	
	assert (pd->f_neurons.size() == pd->f_neuron_pool_size);
	assert (f_neuron_pool.size() == pd->f_neuron_pool_size);
	assert (f_neuron_pool.size() == f_neuron_pool_size);	
	return true; //?
}

void ManagementData::dump_data() {
	//dumping new numbers
	printf ("     actual military_numbers (%lu):\n      {", pd->magic_numbers.size());
	uint16_t itemcounter = 1;
	uint16_t line_counter = 1;
	for (const auto& item : pd->magic_numbers) {
		printf (" %3d %s",item,(&item != &pd->magic_numbers.back())?", ":"");
		if (itemcounter % 10 == 0) {
			printf (" //AutoContent_%02d\n       ", line_counter);
			line_counter +=1;
		}
		++itemcounter;
	}
	printf ("}\n");
	
	printf ("     actual neuron setup:\n      ");
	printf ("{ ");
	itemcounter = 1;
	for (auto& item : neuron_pool) {
		printf (" %3d %s",item.get_weight(),(&item != &neuron_pool.back())?", ":"");
		if (itemcounter % 10 == 0) {
			printf (" //AutoContent_%02d\n       ", line_counter);
			line_counter +=1;
		}
		++itemcounter;
	}
	printf ("}\n      { ");
	itemcounter = 1;	
	for (auto& item : neuron_pool) {
		printf (" %3d %s",item.get_type(),(&item != &neuron_pool.back())?", ":"");
		if (itemcounter % 10 == 0) {
			printf (" //AutoContent_%02d\n       ", line_counter);
			line_counter +=1;
		}
		++itemcounter;
	}
	printf ("}\n");


	printf ("     actual f-neuron setup:\n      ");
	printf ("{ ");
	itemcounter = 1;
	for (auto& item : f_neuron_pool) {
		printf (" %8u %s",item.get_int(),(&item != &f_neuron_pool.back())?", ":"");
		if (itemcounter % 10 == 0) {
			printf (" //AutoContent_%02d\n       ", line_counter);
			line_counter +=1;
		}
		++itemcounter;
	}
	printf ("}\n");
}

int16_t ManagementData::get_military_number_at(uint8_t pos) {
	assert (pos < magic_numbers_size);
	return pd->magic_numbers[pos];
}

void ManagementData::set_military_number_at(const uint8_t pos, const int16_t value) {
	assert (pos < magic_numbers_size);
	assert (pos < pd->magic_numbers.size());
	assert (value >= -100 && value <= 100);
	pd->magic_numbers[pos] = value;
}

uint16_t MineTypesObserver::total_count() const {
	return in_construction + finished;
}

// this is used to count militarysites by their size
MilitarySiteSizeObserver::MilitarySiteSizeObserver() : in_construction(0), finished(0) {
}

// this represents a scheduler task
SchedulerTask::SchedulerTask(const uint32_t time,
                             const Widelands::SchedulerTaskId t,
                             const uint8_t p,
                             const char* d)
   : due_time(time), id(t), priority(p), descr(d) {
}

bool SchedulerTask::operator<(SchedulerTask other) const {
	return priority > other.priority;
}

// List of blocked fields with block time, with some accompanying functions
void BlockedFields::add(Widelands::Coords coords, uint32_t till) {
	const uint32_t hash = coords.hash();
	if (blocked_fields_.count(hash) == 0) {
		blocked_fields_.insert(std::pair<uint32_t, uint32_t>(hash, till));
	} else if (blocked_fields_[hash] < till) {
		blocked_fields_[hash] = till;
	}
	// The third possibility is that a field has been already blocked for longer time than 'till'
}

uint32_t BlockedFields::count() {
	return blocked_fields_.size();
}

void BlockedFields::remove_expired(uint32_t gametime) {
	std::vector<uint32_t> fields_to_remove;
	for (auto field : blocked_fields_) {
		if (field.second < gametime) {
			fields_to_remove.push_back(field.first);
		}
	}
	while (!fields_to_remove.empty()) {
		blocked_fields_.erase(fields_to_remove.back());
		fields_to_remove.pop_back();
	}
}

bool BlockedFields::is_blocked(Coords coords) {
	return (blocked_fields_.count(coords.hash()) != 0);
}

FlagsForRoads::Candidate::Candidate(uint32_t coords, int32_t distance, bool economy)
   : coords_hash(coords), air_distance(distance), different_economy(economy) {
	new_road_possible = false;
	accessed_via_roads = false;
	// Values are only very rough, and are dependant on the map size
	new_road_length = 2 * Widelands::kMapDimensions.at(Widelands::kMapDimensions.size() - 1);
	current_roads_distance = 2 * (Widelands::kMapDimensions.size() - 1);  // must be big enough
	reduction_score = -air_distance;  // allows reasonable ordering from the start
}

bool FlagsForRoads::Candidate::operator<(const Candidate& other) const {
	if (reduction_score == other.reduction_score) {
		return coords_hash < other.coords_hash;
	} else {
		return reduction_score > other.reduction_score;
	}
}

bool FlagsForRoads::Candidate::operator==(const Candidate& other) const {
	return coords_hash == other.coords_hash;
}

void FlagsForRoads::Candidate::calculate_score() {
	if (!new_road_possible) {
		reduction_score = kRoadNotFound - air_distance;  // to have at least some ordering preserved
	} else if (different_economy) {
		reduction_score = kRoadToDifferentEconomy - air_distance - 2 * new_road_length;
	} else if (!accessed_via_roads) {
		if (air_distance + 6 > new_road_length) {
			reduction_score = kShortcutWithinSameEconomy - air_distance - 2 * new_road_length;
		} else {
			reduction_score = kRoadNotFound;
		}
	} else {
		reduction_score = current_roads_distance - 2 * new_road_length;
	}
}

void FlagsForRoads::print() {  // this is for debugging and development purposes
	for (auto& candidate_flag : queue) {
		log("   %starget: %3dx%3d, saving: %5d (%3d), air distance: %3d, new road: %6d, score: %5d "
		    "%s\n",
		    (candidate_flag.reduction_score >= min_reduction && candidate_flag.new_road_possible) ?
		       "+" :
		       " ",
		    Coords::unhash(candidate_flag.coords_hash).x,
		    Coords::unhash(candidate_flag.coords_hash).y,
		    candidate_flag.current_roads_distance - candidate_flag.new_road_length, min_reduction,
		    candidate_flag.air_distance, candidate_flag.new_road_length,
		    candidate_flag.reduction_score,
		    (candidate_flag.new_road_possible) ? ", new road possible" : " ");
	}
}

// Queue is ordered but some target flags are only estimations so we take such a candidate_flag
// first
bool FlagsForRoads::get_best_uncalculated(uint32_t* winner) {
	for (auto& candidate_flag : queue) {
		if (!candidate_flag.new_road_possible) {
			*winner = candidate_flag.coords_hash;
			return true;
		}
	}
	return false;
}

// Road from starting flag to this flag can be built
void FlagsForRoads::road_possible(Widelands::Coords coords, uint32_t distance) {
	// std::set does not allow updating
	Candidate new_candidate_flag = Candidate(0, 0, false);
	for (auto candidate_flag : queue) {
		if (candidate_flag.coords_hash == coords.hash()) {
			new_candidate_flag = candidate_flag;
			assert(new_candidate_flag.coords_hash == candidate_flag.coords_hash);
			queue.erase(candidate_flag);
			break;
		}
	}

	new_candidate_flag.new_road_length = distance;
	new_candidate_flag.new_road_possible = true;
	new_candidate_flag.calculate_score();
	queue.insert(new_candidate_flag);
}

// Remove the flag from candidates as interconnecting road is not possible
void FlagsForRoads::road_impossible(Widelands::Coords coords) {
	const uint32_t hash = coords.hash();
	for (auto candidate_flag : queue) {
		if (candidate_flag.coords_hash == hash) {
			queue.erase(candidate_flag);
			return;
		}
	}
}

// Updating walking distance over existing roads
// Queue does not allow modifying its members so we erase and then eventually insert modified member
void FlagsForRoads::set_road_distance(Widelands::Coords coords, int32_t distance) {
	const uint32_t hash = coords.hash();
	Candidate new_candidate_flag = Candidate(0, 0, false);
	bool replacing = false;
	for (auto candidate_flag : queue) {
		if (candidate_flag.coords_hash == hash) {
			assert(!candidate_flag.different_economy);
			if (distance < candidate_flag.current_roads_distance) {
				new_candidate_flag = candidate_flag;
				queue.erase(candidate_flag);
				replacing = true;
				break;
			}
			break;
		}
	}
	if (replacing) {
		new_candidate_flag.current_roads_distance = distance;
		new_candidate_flag.accessed_via_roads = true;
		new_candidate_flag.calculate_score();
		queue.insert(new_candidate_flag);
	}
}

bool FlagsForRoads::get_winner(uint32_t* winner_hash, uint32_t pos) {
	assert(pos == 1 || pos == 2);
	uint32_t counter = 1;
	// If AI can ask for 2nd position, but there is only one viable candidate
	// we return the first one of course
	bool has_winner = false;
	for (auto candidate_flag : queue) {
		if (candidate_flag.reduction_score < min_reduction || !candidate_flag.new_road_possible) {
			continue;
		}
		assert(candidate_flag.air_distance > 0);
		assert(candidate_flag.reduction_score >= min_reduction);
		assert(candidate_flag.new_road_possible);
		*winner_hash = candidate_flag.coords_hash;
		has_winner = true;

		if (counter == pos) {
			return true;
		} else if (counter < pos) {
			counter += 1;
		} else {
			break;
		}
	}
	if (has_winner) {
		return true;
	}
	return false;
}

// This is an struct that stores strength of players, info on teams and provides some outputs from
// these data
PlayersStrengths::PlayerStat::PlayerStat() : team_number(0), is_enemy(false), players_power(0), old_players_power(0), old60_players_power(0),players_casualities(0) {
}
PlayersStrengths::PlayerStat::PlayerStat(Widelands::TeamNumber tc, bool e, uint32_t pp, uint32_t op, uint32_t o60p, uint32_t cs, uint32_t land, uint32_t oland)
   : team_number(tc), is_enemy(e), players_power(pp),  old_players_power(op), old60_players_power(o60p), players_casualities(cs), players_land(land), old_players_land(oland)  {
	 last_time_seen = kNever;  
}

// Inserting/updating data
void PlayersStrengths::add(Widelands::PlayerNumber pn, Widelands::PlayerNumber opn, Widelands::TeamNumber mytn,
   Widelands::TeamNumber pltn, uint32_t pp, uint32_t op, uint32_t o60p, uint32_t cs, uint32_t land, uint32_t oland) {
	if (all_stats.count(opn) == 0) {
		bool enemy = false;
		if ( pn == opn ) {
			;
		} else if (pltn == 0 || mytn == 0) {
			enemy = true;
		} else if (pltn != mytn) {
			enemy = true;			
		}
		this_player_number = pn;
		printf (" %d PlayersStrengths: player %d / team: %d - is%s enemy\n", pn, opn, pltn, (enemy)?"":" not");
		all_stats.insert(std::pair<Widelands::PlayerNumber, PlayerStat>(opn, PlayerStat(pltn, enemy, pp, op, o60p, cs, land, oland)));
	} else {
		all_stats[opn].players_power = pp;
		all_stats[opn].old_players_power = op;
		all_stats[opn].old60_players_power = o60p;
		all_stats[opn].players_casualities = cs;
		all_stats[opn].players_land = land;
		all_stats[opn].old_players_land = oland;
	}
}

void PlayersStrengths::recalculate_team_power() {
	team_powers.clear();
	for (auto& item : all_stats) {
		if (item.second.team_number > 0) {  // is a member of a team
			if (team_powers.count(item.second.team_number) > 0) {
				team_powers[item.second.team_number] += item.second.players_power;
			} else {
				team_powers[item.second.team_number] = item.second.players_power;
			}
		}
	}
}

bool PlayersStrengths::any_enemy_seen_lately(const uint32_t gametime){
	for (auto& item : all_stats) {
		if (item.second.is_enemy && player_seen_lately(item.first, gametime)) {
			return true;
		}  // is a member of a team
	}
	return false;
}

uint8_t PlayersStrengths::enemies_seen_lately_count(const uint32_t gametime){
	uint8_t count = 0;
	for (auto& item : all_stats) {
		if (item.second.is_enemy && player_seen_lately(item.first, gametime)) {
			count +=1;
		}  // is a member of a team
	}
	return count;
}

uint32_t PlayersStrengths::enemy_last_seen(){
	uint32_t time = 0;
	for (auto& item : all_stats) {
		if (item.second.last_time_seen == kNever){
			continue;
		}
		if (item.second.is_enemy && item.second.last_time_seen > time) {
			time = item.second.last_time_seen;
		}  
	}
	if (time == 0) {return kNever;}
	return time;
}



void PlayersStrengths::set_last_time_seen(const uint32_t seentime, Widelands::PlayerNumber pn){
	assert(all_stats.count(pn) > 0);
	all_stats[pn].last_time_seen = seentime;
}

bool PlayersStrengths::get_is_enemy(Widelands::PlayerNumber pn){
	assert(all_stats.count(pn) > 0);
	return all_stats[pn].is_enemy;

}

bool PlayersStrengths::player_seen_lately( Widelands::PlayerNumber pn, const uint32_t gametime){
	assert(all_stats.count(pn) > 0);
	if (all_stats[pn].last_time_seen == kNever){
		return false;
	}
	if (all_stats[pn].last_time_seen + static_cast<uint32_t>(2 * 60 * 1000) > gametime){
		return true;
	}
	return false;
}

// This is strength of player
uint32_t PlayersStrengths::get_player_power(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].players_power;
	};
	return 0;
}

// This is land size owned by player
uint32_t PlayersStrengths::get_player_land(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].players_land;
	};
	return 0;
}

uint32_t PlayersStrengths::get_visible_enemies_power(const uint32_t gametime){
	uint32_t pw = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first) && player_seen_lately(item.first, gametime)) {
			pw += item.second.players_power;
		}
	}
	return pw;
}

uint32_t PlayersStrengths::get_enemies_average_power(){
	uint32_t sum = 0;
	uint8_t count = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first)) {
			sum += item.second.players_power;
			count += 1;
		}
	}
	if (count > 0) {
		return sum/count;
	}
	return 0;
}

uint32_t PlayersStrengths::get_enemies_average_land(){
	uint32_t sum = 0;
	uint8_t count = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first)) {
			sum += item.second.players_land;
			count += 1;
		}
	}
	if (count > 0) {
		return sum/count;
	}
	return 0;
}

uint32_t PlayersStrengths::get_enemies_max_power(){
	uint32_t power = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first)) {
			power=std::max<uint32_t>(power, item.second.players_power);
		}
	}
	return power;
}

uint32_t PlayersStrengths::get_enemies_max_land(){
	uint32_t land = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first)) {
			land=std::max<uint32_t>(land, item.second.players_land);
		}
	}
	return land;
}

uint32_t PlayersStrengths::get_old_player_power(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].old_players_power;
	};
	return 0;
}

uint32_t PlayersStrengths::get_old60_player_power(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].old60_players_power;
	};
	return 0;
}

uint32_t PlayersStrengths::get_old_player_land(Widelands::PlayerNumber pn) {
	assert(all_stats.count(pn) > 0);
	return all_stats[pn].old_players_land;
}


uint32_t PlayersStrengths::get_old_visible_enemies_power(const uint32_t gametime){
	uint32_t pw = 0;
	for (auto& item : all_stats) {
		if (get_is_enemy(item.first) && player_seen_lately(item.first, gametime)) {
			pw += item.second.old_players_power;
		}
	}
	return pw;
}

// This is casualities of player
uint32_t PlayersStrengths::get_player_casualities(Widelands::PlayerNumber pn) {
	if (all_stats.count(pn) > 0) {
		return all_stats[pn].players_casualities;
	};
	return 0;
}

// This is strength of player plus third of strength of other members of his team
uint32_t PlayersStrengths::get_modified_player_power(Widelands::PlayerNumber pn) {
	uint32_t result = 0;
	Widelands::TeamNumber team = 0;
	if (all_stats.count(pn) > 0) {
		result = all_stats[pn].players_power;
		team = all_stats[pn].team_number;
	};
	if (team > 0 && team_powers.count(team) > 0) {
		result = result + (team_powers[team] - result) / 3;
	};
	return result;
}

bool PlayersStrengths::players_in_same_team(Widelands::PlayerNumber pl1,
                                            Widelands::PlayerNumber pl2) {
	assert(all_stats.count(pl1) > 0);
	assert(all_stats.count(pl2) > 0);
	if (pl1 == pl2) {
		return false;
	} else if (all_stats[pl1].team_number > 0 &&
		       all_stats[pl1].team_number == all_stats[pl2].team_number) {
		// team number 0 = no team
		return true;
	} else {
		return false;
	}
}

bool PlayersStrengths::strong_enough(Widelands::PlayerNumber pl) {
	if (all_stats.count(pl) == 0) {
		return false;
	}
	uint32_t my_strength = all_stats[pl].players_power;
	uint32_t strongest_opponent_strength = 0;
	for (auto item : all_stats) {
		if (!players_in_same_team(item.first, pl) && pl != item.first) {
			if (get_modified_player_power(item.first) > strongest_opponent_strength) {
				strongest_opponent_strength = get_modified_player_power(item.first);
			}
		}
	}
	return my_strength > strongest_opponent_strength + 50;
}

}  // namespace WIdelands
