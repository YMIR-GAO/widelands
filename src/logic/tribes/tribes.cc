/*
 * Copyright (C) 2006-2014 by the Widelands Development Team
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

#include "logic/tribes/tribes.h"

#include "logic/constructionsite.h"
#include "logic/dismantlesite.h"
#include "logic/militarysite.h"
#include "logic/productionsite.h"
#include "logic/trainingsite.h"
#include "logic/ware_descr.h"
#include "logic/worker_descr.h"

namespace Widelands {

Tribes::Tribes(EditorGameBase& egbase) :
	buildings_(new DescriptionMaintainer<BuildingDescr>()),
	wares_(new DescriptionMaintainer<WareDescr>()),
	workers_(new DescriptionMaintainer<WorkerDescr>()) {
}

void Tribes::add_constructionsite_type(const LuaTable& t) {
	buildings_->add(new ConstructionSiteDescr(t));
}

void Tribes::add_dismantlesite_type(const LuaTable& t) {
	buildings_->add(new DismantleSiteDescr(t));
}

void Tribes::add_militarysite_type(const LuaTable& t) {
	buildings_->add(new MilitarySiteDescr(t));
}

void Tribes::add_productionsite_type(const LuaTable& t) {
	buildings_->add(new ProductionSiteDescr(t, egbase.world()));
}

void Tribes::add_trainingsite_type(const LuaTable& t) {
	buildings_->add(new TrainingSiteDescr(t, egbase.world()));
}


void Tribes::add_ware_type(const LuaTable& t) {
	wares_->add(new WareDescr(t));
}

void Tribes::add_worker_type(const LuaTable& t) {
	workers_->add(new WorkerDescr(t));
}


WareIndex Tribes::get_nrwares() const {
	return wares_.size();
}

WareIndex Tribes::get_nrworkers() const {
	return workers_.size();
}


BuildingIndex Tribes::safe_building_index(const std::string& buildingname) const {
	const BuildingIndex result = building_index(buildingname);
	if (result == -1) {
		throw GameDataError("Unknown building type \"%s\"", buildingname.c_str());
	}
	return result;
}

WareIndex Tribes::safe_ware_index(const std::string& warename) const {
	const WareIndex result = ware_index(warename);
	if (result == -1) {
		throw GameDataError("Unknown ware type \"%s\"", warename.c_str());
	}
	return result;
}

WareIndex Tribes::safe_worker_index(const std::string& workername) const {
	const WareIndex result = worker_index(workername);
	if (result == -1) {
		throw GameDataError("Unknown worker type \"%s\"", workername.c_str());
	}
	return result;
}

BuildingIndex Tribes::building_index(const std::string& buildingname) const {
	int result = -1;
	for (size_t i = 0; i < buildings_.size(); ++i) {
		if (buildings_.get(i)->name() == buildingname.name()) {
			return result;
		}
	}
}

WareIndex Tribes::ware_index(const std::string& warename) const {
	int result = -1;
	for (size_t i = 0; i < wares_.size(); ++i) {
		if (wares_.get(i)->name() == warename.name()) {
			return result;
		}
	}
}

WareIndex Tribes::worker_index(const std::string& workername) const {
	int result = -1;
	for (size_t i = 0; i < workers_.size(); ++i) {
		if (workers_.get(i)->name() == workername.name()) {
			return result;
		}
	}
}

BuildingDescr const * Tribes::get_building_descr(BuildingIndex building_index) const {
	return buildings_.get(building_index);
}

WareDescr const * Tribes::get_ware_descr(WareIndex ware_index) const {
	return wares_.get(ware_index);
}

WorkerDescr const * Tribes::get_worker_descr(WareIndex worker_index) const {
	return workers_.get(worker_index);
}


void Tribes::set_ware_type_has_demand_check(WareIndex ware_index, const std::string& tribename) const {
	wares_.get(ware_index)->set_has_demand_check(tribename);
}

void Tribes::set_worker_type_has_demand_check(WareIndex worker_index, const std::string& tribename) const {
	workers_.get(worker_index)->set_has_demand_check(tribename);
}

} // namespace Widelands
