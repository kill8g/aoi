#include "../libs/aoi.h"
#include <random>
#include <unordered_map>

std::unordered_map<int, const char*> defines = {
	{AOI::entity_action::ENTER, "enter view"},
	{AOI::entity_action::LEAVE, "leave view"},
	{AOI::entity_action::IDLE, "idle"},
	{AOI::entity_action::MOVE, "move"},
};
void on_entity_action(AOI::entity* who, AOI::entity_action action, AOI::watcher* watch) {
	fprintf(stderr, "entity<%llu>recv msg, <%llu> <%s>, pos<%d, %d>\n",
		watch->_uid,
		who->_uid,
		defines[action],
		who->_pos._x,
		who->_pos._y);
}
int main(int argc, char* argv[]) {
	std::random_device seed;
	std::ranlux48 engine(seed());
	std::uniform_int_distribution<> random(0, 100);

	// create aoi
	auto action_event = std::bind(
		on_entity_action,
		std::placeholders::_1,
		std::placeholders::_2,
		std::placeholders::_3
	);
	const int grid_wide = 3;
	const int grid_hige = 3;
	AOI::grid_aoi aoi(grid_wide, grid_hige, action_event);

	// enter
	for (int uid = 0; uid < 3; ++uid) {
		const int pos_x = 0;
		const int pos_y = 0;
		aoi.on_entity_action(uid, AOI::entity_action::ENTER, pos_x, pos_y);

	}
	// move
	for (int uid = 0; uid < 3; ++uid) {
		AOI::entity* e = aoi.get_entity(uid);
		if (e) {
			int i = random(engine) % sizeof(AOI::path_list) / sizeof(AOI::position);
			const AOI::position& path = AOI::path_list[i];
			AOI::position pos = e->_pos + path;
			aoi.on_entity_action(uid, AOI::entity_action::MOVE, pos._x, pos._y);
		}
	}
	// leave
	for (int uid = 0; uid < 3; ++uid) {
		aoi.on_entity_action(uid, AOI::entity_action::LEAVE);

	}
	return 0;
}
