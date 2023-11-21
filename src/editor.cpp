#include "editor.hpp"
#include "render.hpp"

namespace digital_circuit_maker{
void Editor::process_movement(const sf::RenderWindow& render_window, 
	const sf::Vector2u map_size){

	sf::Vector2i mpos = sf::Mouse::getPosition(render_window);
	cursor_cell.position = (sf::Vector2u)render_window.mapPixelToCoords(mpos);
	cursor_cell.position /= Render::get_instance().CELL_SIZE;

	bool x_ok = cursor_cell.position.x < map_size.x;
	bool y_ok = cursor_cell.position.y < map_size.y;
	cursor_cell.valid_position = x_ok && y_ok;
}

void Editor::process_rotation(){
	static bool last_key_state = false;

	if(sf::Keyboard::isKeyPressed(sf::Keyboard::R) && !last_key_state){
		sf::Vector2i new_dir, dir = cursor_cell.direction;

		if(dir == sf::Vector2i(0, -1)) new_dir = sf::Vector2i(-1, 0);
		else if(dir == sf::Vector2i(-1, 0)) new_dir = sf::Vector2i(0, 1);
		else if(dir == sf::Vector2i(0, 1)) new_dir = sf::Vector2i(1, 0);
		else new_dir = sf::Vector2i(0, -1);

		cursor_cell.direction = new_dir;
	}

	last_key_state = sf::Keyboard::isKeyPressed(sf::Keyboard::R);
}

void Editor::process_placement(Map& map) const{
	bool left_pressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
	bool right_pressed = sf::Mouse::isButtonPressed(sf::Mouse::Right);
	if(left_pressed && !right_pressed){
		if(cursor_cell.valid_position) 
			if(cursor_cell.type != Map::Cell::Type::EMPTY) place_cell(map);
	}
}

void Editor::process_removal(Map& map) const{
	bool left_pressed = sf::Mouse::isButtonPressed(sf::Mouse::Left);
	bool right_pressed = sf::Mouse::isButtonPressed(sf::Mouse::Right);
	if(right_pressed && !left_pressed){
		if(cursor_cell.valid_position) remove_cell(map);
	}
}

void Editor::set_cursor_cell_type(const Map::Cell::Type type){
	cursor_cell.type = type;
}

const Editor::CursorCell& Editor::get_cursor_cell() const{
	return cursor_cell;
}

void Editor::place_cell(Map& map) const{
	sf::Vector2u pos = cursor_cell.position;

	bool same_type = map.cells[pos.x][pos.y].type == cursor_cell.type;
	bool same_direction = map.cells[pos.x][pos.y].direction == cursor_cell.direction;
	if(same_type && same_direction) return;

	Map::Cell new_cell = {
		.type = cursor_cell.type,
		.wire_group = NULL,
		.behaviour = NULL,
		.direction = cursor_cell.direction,
		.last_out = false
	};

	map.cells[pos.x][pos.y] = new_cell;
}

void Editor::remove_cell(Map& map) const{
	sf::Vector2u pos = cursor_cell.position;

	if(map.cells[pos.x][pos.y].type == Map::Cell::Type::EMPTY) return;

	Map::Cell new_cell = {
		.type = Map::Cell::Type::EMPTY,
		.wire_group = NULL,
		.behaviour = NULL,
		.direction = sf::Vector2i(0, 0),
		.last_out = false
	};

	map.cells[pos.x][pos.y] = new_cell;
}

Editor::Editor(){

}
}