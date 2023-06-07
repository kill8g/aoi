#ifndef _TYPES_H_
#define _TYPES_H_

#include <vector>
#include <set>
#include <unordered_map>
#include <math.h>

namespace AOI {
	typedef unsigned long long uint64;

	struct position {
		position(int x = 0, int y = 0) {
			this->_x = x;
			this->_y = y;
		}
		position(const position& other) {
			this->_x = other._x;
			this->_y = other._y;
		}
		position& operator+=(const position& other) {
			this->_x += other._x;
			this->_y += other._y;
			return *this;
		}
		position operator+(const position& other) {
			position pos(other._x + this->_x, other._y + this->_y);
			return pos;
		}
		position& operator++() {
			++(this->_x);
			++(this->_y);
			return *this;
		}
		position& operator--() {
			--(this->_x);
			--(this->_y);
			return *this;
		}

		double operator-(const position& other) {
			int dx = this->_x - other._x;
			int dy = this->_y - other._y;
			return sqrt(pow(dx, 2) + pow(dy, 2));
		}
		position& operator=(const position& other) {
			if (this != &other) {
				this->_x = other._x;
				this->_y = other._y;
			}
			return *this;
		}
		int _x;
		int _y;
	};

	struct entity {
		entity(uint64 uid, const position& pos) {
			this->_uid = uid;
			this->_pos = pos;
		}
		uint64 _uid;
		position _pos;
	};

	struct map_grid {
		map_grid(int x, int y) {
			this->_index_x = x;
			this->_index_y = y;
		}

		map_grid(const map_grid&) = delete;
		map_grid& operator=(const map_grid&) = delete;

		bool operator==(const map_grid& other) {
			return this->_index_x == other._index_x && this->_index_y == other._index_y;
		}
		int _index_x;
		int _index_y;
		std::set<uint64> _uids;
	};

	enum entity_action {
		ENTER,
		LEAVE,
		IDLE,
		MOVE,
	};

	typedef std::set<uintptr_t> addr_set;
	typedef std::unordered_map<int, map_grid*> grid_dict;
	typedef entity watcher;

	const position path_list[] = {
		position(-1, -1),	position(0, -1),	position(1, -1),
		position(-1, 0),	position(0, 0),		position(1, 0),
		position(-1, 1),	position(0, 1),		position(1, 1)
	};
}

#define pos2grid_index(pos, size, v) pos._##v / size

#endif // !_TYPES_H_

