#ifndef _GRID_AOI_H_
#define _GRID_AOI_H_

#include "types.h"
#include <functional>

namespace AOI {
	class grid_aoi {
	public:
		grid_aoi(int grid_wide, int grid_hige, const std::function<void(entity*, entity_action, watcher*)>& event);
		~grid_aoi();
		grid_aoi(const grid_aoi&) = delete;
		grid_aoi& operator=(const grid_aoi&) = delete;

		void on_entity_action(uint64 uid, entity_action action, ...);

	protected:
		void on_entity_enter(uint64 uid, const position& pos);
		void on_entity_leave(uint64 uid);
		void on_entity_move(uint64 uid, const position& pos);
		void on_entity_idle(uint64 uid);
		void on_change_grid(map_grid* before, map_grid* after, addr_set& before_grids, addr_set& after_grids, addr_set& cross_grids);
		map_grid* get_grid(const position& pos);
		map_grid* new_grid(int index_x, int index_y);
		void broadcast_near(const position& pos, entity_action action, entity* who);
	public:
		entity* get_entity(uint64 uid);

	private:
		int _wide;
		int _hige;
		std::unordered_map<uint64, entity*> _entitys;
		std::unordered_map<int, grid_dict> _grids;
		std::function<void(entity*, entity_action, watcher*)> _action_event;
	};
}

#endif // !_GRID_AOI_H_

