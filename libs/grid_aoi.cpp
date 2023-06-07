#include "grid_aoi.h"
#include <stdarg.h>

AOI::grid_aoi::grid_aoi(int wide, int hige, const std::function<void(entity*, entity_action, watcher*)>& event) {
	this->_wide = wide;
	this->_hige = hige;
	this->_action_event = event;
}

void AOI::grid_aoi::on_entity_action(uint64 uid, entity_action action, ...) {
	switch (action) {
	case entity_action::ENTER:
	{
		va_list args;
		va_start(args, action);
		const int pos_x = va_arg(args, int);
		const int pos_y = va_arg(args, int);
		va_end(args);
		position pos(pos_x, pos_y);
		this->on_entity_enter(uid, pos);
	}
	break;
	case entity_action::LEAVE:
	{
		this->on_entity_leave(uid);
	}
	break;
	case entity_action::IDLE:
	{
		this->on_entity_leave(uid);
	}
	break;
	case entity_action::MOVE:
	{
		va_list args;
		va_start(args, action);
		const int pos_x = va_arg(args, int);
		const int pos_y = va_arg(args, int);
		va_end(args);
		position pos(pos_x, pos_y);
		this->on_entity_move(uid, pos);
	}
	break;
	}
}

void AOI::grid_aoi::on_entity_enter(uint64 uid, const position& pos) {
	auto iter = this->_entitys.find(uid);
	if (iter != this->_entitys.end())
		return;

	map_grid* grid = this->get_grid(pos);
	if (!grid) {
		int index_x = pos2grid_index(pos, this->_wide, x);
		int index_y = pos2grid_index(pos, this->_hige, y);
		grid = this->new_grid(index_x, index_y);
	}

	entity* e = new entity(uid, pos);
	grid->_uids.insert(uid);
	this->_entitys.insert(std::make_pair(uid, e));

	this->broadcast_near(pos, entity_action::ENTER, e);
}

void AOI::grid_aoi::on_entity_leave(uint64 uid) {
	auto iter = this->_entitys.find(uid);
	if (iter == this->_entitys.end()) {
		fprintf(stderr, "[on_entity_leave] not found entity <%llu>", uid);
		return;
	}

	entity* e = iter->second;
	if (!e) {
		fprintf(stderr, "[on_entity_leave] entity <%llu> is nullptr", uid);
		return;
	}

	map_grid* grid = this->get_grid(e->_pos);
	if (!grid) {
		fprintf(stderr, "[on_entity_leave] before is nullptr, entity uid <%llu>, pos <%d, %d>", e->_uid, e->_pos._x, e->_pos._y);
		return;
	}

	this->broadcast_near(e->_pos, entity_action::LEAVE, e);

	grid->_uids.erase(uid);
	this->_entitys.erase(uid);
	delete e;
	e = nullptr;
}

void AOI::grid_aoi::on_entity_move(uint64 uid, const position& pos) {
	auto iter = this->_entitys.find(uid);
	if (iter == this->_entitys.end()) {
		fprintf(stderr, "[on_entity_move] not found entity <%llu>", uid);
		return;
	}

	entity* e = iter->second;
	if (!e) {
		fprintf(stderr, "[on_entity_move] entity <%llu> is nullptr", uid);
		return;
	}

	map_grid* before = this->get_grid(e->_pos);
	if (!before) {
		fprintf(stderr, "[on_entity_move] before is nullptr, entity uid <%llu>, pos <%d, %d>", e->_uid, e->_pos._x, e->_pos._y);
		return;
	}

	map_grid* after = this->get_grid(pos);
	if (!after) {
		after = this->new_grid(pos2grid_index(pos, this->_wide, x), pos2grid_index(pos, this->_hige, y));
	}

	e->_pos = pos;

	if (*before == *after) {
		this->broadcast_near(pos, entity_action::MOVE, e);
	}
	else {
		addr_set before_grids, after_grids, cross_grids;
		this->on_change_grid(before, after, before_grids, after_grids, cross_grids);

		std::vector<entity*> entitys;

		// 更新视野
		for (auto& addr : cross_grids) {
			map_grid* grid = (map_grid*)addr;
			for (auto& uid : grid->_uids) {
				entity* watch = this->get_entity(uid);
				if (watch)
					entitys.push_back(watch);
			}
		}
		for (auto& watch : entitys) {
			this->_action_event(e, entity_action::MOVE, watch);
		}
		entitys.clear();

		// 移动到新的格子内
		before->_uids.erase(e->_uid);
		after->_uids.insert(e->_uid);

		// 进入视野
		for (auto& addr : after_grids) {
			map_grid* grid = (map_grid*)addr;
			for (auto& uid : grid->_uids) {
				entity* watch = this->get_entity(uid);
				if (watch && watch->_uid != e->_uid) {
					entitys.push_back(watch);
				}
			}
		}
		for (auto& watch : entitys) {
			this->_action_event(e, entity_action::ENTER, watch);
			this->_action_event(watch, entity_action::ENTER, e);
		}
		entitys.clear();

		// 离开视野
		for (auto& addr: before_grids) {
			map_grid* grid = (map_grid*)addr;
			for (auto& uid : grid->_uids) {
				entity* watch = this->get_entity(uid);
				if (watch && watch->_uid != e->_uid) {
					entitys.push_back(watch);
				}
			}
		}
		for (auto& watch : entitys) {
			this->_action_event(e, entity_action::LEAVE, watch);
			this->_action_event(watch, entity_action::LEAVE, e);
		}
		entitys.clear();
	}
}

void AOI::grid_aoi::on_entity_idle(uint64 uid) {
	auto iter = this->_entitys.find(uid);
	if (iter == this->_entitys.end()) {
		fprintf(stderr, "[on_entity_idle] not found entity <%llu>", uid);
		return;
	}

	entity* e = iter->second;
	if (!e) {
		fprintf(stderr, "[on_entity_idle] entity <%llu> is nullptr", uid);
		return;
	}

	this->broadcast_near(e->_pos, entity_action::LEAVE, e);
}

void AOI::grid_aoi::on_change_grid(map_grid* before, map_grid* after, addr_set& before_grids, addr_set& after_grids, addr_set& cross_grids) {
	// before
	int index_x = before->_index_x;
	int index_y = before->_index_y;
	for (auto& path : path_list) {
		auto iter_x = this->_grids.find(index_x + path._x);
		if (iter_x == this->_grids.end())
			continue;

		grid_dict& grids = iter_x->second;
		auto iter = grids.find(index_y + path._y);
		if (iter == grids.end())
			continue;

		map_grid* grid = iter->second;
		uintptr_t addr = (uintptr_t)grid;
		before_grids.insert(addr);
	}

	// after
	index_x = after->_index_x;
	index_y = after->_index_y;
	for (auto& path : path_list) {
		auto iter_x = this->_grids.find(index_x + path._x);
		if (iter_x == this->_grids.end())
			continue;

		grid_dict& grids = iter_x->second;
		auto iter = grids.find(index_y + path._y);
		if (iter == grids.end())
			continue;

		map_grid* grid = iter->second;
		uintptr_t addr = (uintptr_t)grid;
		if (before_grids.count(addr)) {
			before_grids.erase(addr);
			cross_grids.insert(addr);
		}
		else {
			after_grids.insert(addr);
		}
	}
}

AOI::map_grid* AOI::grid_aoi::get_grid(const position& pos) {
	int index_x = pos2grid_index(pos, this->_wide, x);
	int index_y = pos2grid_index(pos, this->_hige, y);
	auto iter_x = this->_grids.find(index_x);
	if (iter_x == this->_grids.end())
		return nullptr;
	grid_dict& grids = iter_x->second;
	auto iter_y = grids.find(index_y);
	if (iter_y == grids.end())
		return nullptr;
	return iter_y->second;
}

AOI::map_grid* AOI::grid_aoi::new_grid(int index_x, int index_y) {
	map_grid* grid = new map_grid(index_x, index_y);
	if (this->_grids.find(index_x) == this->_grids.end()) {
		this->_grids.insert(std::make_pair(index_x, std::unordered_map<int, map_grid*>()));
	}
	this->_grids[index_x][index_y] = grid;
	return grid;
}

void AOI::grid_aoi::broadcast_near(const position& pos, entity_action action, entity* who) {
	int index_x = pos2grid_index(pos, this->_wide, x);
	int index_y = pos2grid_index(pos, this->_hige, y);
	for (auto& path : path_list) {
		auto iter_x = this->_grids.find(index_x + path._x);
		if (iter_x == this->_grids.end())
			continue;

		grid_dict& grids = iter_x->second;
		auto iter = grids.find(index_y + path._y);
		if (iter == grids.end())
			continue;

		map_grid* grid = iter->second;
		for (auto& uid : grid->_uids) {
			entity* e = this->get_entity(uid);
			if (e)
				this->_action_event(who, action, e);
		}
	}
}

AOI::entity* AOI::grid_aoi::get_entity(uint64 uid) {
	auto iter = this->_entitys.find(uid);
	if (iter == this->_entitys.end())
		return nullptr;

	entity* e = iter->second;
	return e;
}

AOI::grid_aoi::~grid_aoi() {

}

