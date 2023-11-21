#include "nlohmann/json.hpp"
#include <fstream>
#include <SFML/Graphics.hpp>
#include <cmath>
#include <iostream>

////////////////////////////////////////////////////////////////////////////////

namespace window{
struct Config{
	bool fullscreen;
	sf::Vector2u size;
	bool vsync;
	unsigned int fps_limit; 
};

void create(sf::RenderWindow& window, const Config& cfg){
	const sf::Uint32 style = 
		cfg.fullscreen ? sf::Style::Fullscreen : sf::Style::Default;

	window.create(sf::VideoMode(cfg.size.x, cfg.size.y), 
		"Digital Circuit Maker v0.0", style);
	window.setVerticalSyncEnabled(cfg.vsync);
	window.setFramerateLimit(cfg.fps_limit);
}

void handle_events(sf::RenderWindow& window){
	static sf::Event event;
	while(window.pollEvent(event))
		if(event.type == sf::Event::Closed) window.close();
}
}

////////////////////////////////////////////////////////////////////////////////

class Camera{
public:
	sf::View view;

	Camera(const sf::Vector2u size, const float move_speed, 
		const float zoom_speed){

		this->move_speed = move_speed;
		this->zoom_speed = zoom_speed;

		view = sf::View(sf::FloatRect(0, 0, size.x, size.y));
	}

	void process_movement(const float delta){
		using kb = sf::Keyboard;

		const sf::Vector2f direction = sf::Vector2f(
			kb::isKeyPressed(kb::D) - kb::isKeyPressed(kb::A),
			kb::isKeyPressed(kb::S) - kb::isKeyPressed(kb::W));

		auto normalize = [](sf::Vector2f v){
			float magnitude = sqrt(v.x * v.x + v.y * v.y);
			if(magnitude > 0) v /= magnitude;
			return v;
		};

		view.move(normalize(direction) * move_speed * zoom_value * delta);
	}

	void process_zooming(const float delta){
		using kb = sf::Keyboard;

		const int direction = 
			kb::isKeyPressed(kb::Hyphen) - kb::isKeyPressed(kb::Equal);

		const float factor = 1 + direction * zoom_speed * delta;

		if(factor){
			view.zoom(factor);
			zoom_value *= factor;
		}
	}

private:
	float zoom_value = 1;
	float move_speed, zoom_speed;
};

////////////////////////////////////////////////////////////////////////////////

class Map{
public:
	struct Tile{
		enum Type{
			EMPTY, WIRE, JUNCTION, SWITCH, 
			BUFFER, NOT, OR, NOR, AND, NAND, XOR, XNOR
		}type = EMPTY;
	};

	sf::Vector2u size;
	std::vector<std::vector<Tile>> tiles;

	Map(const sf::Vector2u size){
		this->size = size;

		tiles = std::vector<std::vector<Tile>>(size.x, std::vector<Tile>(size.y));
	}
};

////////////////////////////////////////////////////////////////////////////////

class Editor{
public:
	sf::Vector2u pos;
	std::vector<std::vector<Map::Tile>> tiles;

	void process_movement(const sf::RenderWindow& window){
		sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
		mouse_pos = (sf::Vector2i)window.mapPixelToCoords(mouse_pos);
	}
};

////////////////////////////////////////////////////////////////////////////////

namespace render{
const sf::Color TILE_COLORS[] = {
	sf::Color(0, 0, 0) // EMPTY
};

struct Constraint{
	sf::Vector2u start, end;
};

Constraint get_view_constraint(const sf::View& view, const sf::Vector2u map_size){
	static Constraint constraint;
	static sf::Vector2f last_view_center, last_view_size;

	const bool view_center_same = last_view_center == view.getCenter();
	const bool view_size_same = last_view_size == view.getSize();
	if(view_center_same && view_size_same) return constraint;
	else{
		last_view_center = view.getCenter();
		last_view_size = view.getSize();
	}

	sf::Vector2i start, end;
	start.x = view.getCenter().x - view.getSize().x / 2;
	end.x = start.x + view.getSize().x;
	start.y = view.getCenter().y - view.getSize().y / 2;
	end.y = start.y + view.getSize().y;

	const unsigned int PADDING = 2;
	start.x = start.x - PADDING;
	end.x = end.x + PADDING;
	start.y = start.y - PADDING;
	end.y = end.y + PADDING;

	auto bound = [](const int min, const int value, 
		const int max) -> unsigned int{

		return std::max(min, std::min(value, max));
	};

	constraint.start.x = bound(0, start.x, map_size.x);
	constraint.end.x = bound(0, end.x, map_size.x);
	constraint.start.y = bound(0, start.y, map_size.y);
	constraint.end.y = bound(0, end.y, map_size.y);

	return constraint;
}

inline void add_pixel_to_vertex_array(sf::VertexArray& vertices, const sf::Vector2f pos, 
	const sf::Color color){

	vertices.append(sf::Vertex({pos.x, pos.y}, color));
	vertices.append(sf::Vertex({pos.x + 1, pos.y}, color));
	vertices.append(sf::Vertex({pos.x, pos.y + 1}, color));

	vertices.append(sf::Vertex({pos.x, pos.y + 1}, color));
	vertices.append(sf::Vertex({pos.x + 1, pos.y}, color));
	vertices.append(sf::Vertex({pos.x + 1, pos.y + 1}, color));
}

void draw_map(sf::RenderWindow& window, const Constraint& constraint, 
	const Map& map){

	sf::VertexArray tiles(sf::Triangles);

	for(unsigned int x = constraint.start.x; x < constraint.end.x; x++){
		for(unsigned int y = constraint.start.y; y < constraint.end.y; y++){
			if(map.tiles[x][y].type == Map::Tile::Type::EMPTY) continue;

			const sf::Color color = TILE_COLORS[map.tiles[x][y].type]; 
			add_pixel_to_vertex_array(tiles, sf::Vector2f(x, y), color);
		}
	}

	window.draw(tiles);
}

void draw_grid(sf::RenderWindow& window, const Constraint& constraint){
	const sf::Color GRID_COLOR = sf::Color(100, 100, 100);

	static sf::VertexArray line(sf::Lines, 2);

	for(unsigned int x = constraint.start.x; x < constraint.end.x + 1; x++){
		line[0] = sf::Vertex(sf::Vector2f(x, constraint.start.y), GRID_COLOR);
		line[1] = sf::Vertex(sf::Vector2f(x, constraint.end.y), GRID_COLOR);

		window.draw(line);
	}

	for(unsigned int y = constraint.start.y; y < constraint.end.y + 1; y++){
		line[0] = sf::Vertex(sf::Vector2f(constraint.start.x, y), GRID_COLOR);
		line[1] = sf::Vertex(sf::Vector2f(constraint.end.x, y), GRID_COLOR);

		window.draw(line);
	}
}
}

////////////////////////////////////////////////////////////////////////////////

nlohmann::json get_json_from_file(const std::string& file_path){
	std::ifstream file(file_path);
	return nlohmann::json::parse(file);
}

float get_delta_time(void){
	static sf::Clock clock;
	return clock.restart().asSeconds();
}

int main(){
	nlohmann::json cfg = get_json_from_file("config.json");

	sf::RenderWindow window;

	window::create(window, {cfg["fullscreen"], 
		sf::Vector2u(cfg["size"]["x"], cfg["size"]["y"]), 
		cfg["vsync"], cfg["fps_limit"]});

	Camera camera(sf::Vector2u(cfg["size"]["x"], cfg["size"]["y"]), 
		cfg["camera"]["move_speed"], cfg["camera"]["zoom_speed"]);

	Editor editor;

	Map map(sf::Vector2u(30, 30));

	while(window.isOpen()){
		window::handle_events(window);
		float delta = get_delta_time();

		if(window.hasFocus()){
			camera.process_movement(delta);
			camera.process_zooming(delta);

			editor.process_movement(window);
		}

		const render::Constraint constraint = 
			render::get_view_constraint(camera.view, map.size);

		window.setView(camera.view);
		window.clear(render::TILE_COLORS[Map::Tile::Type::EMPTY]);
			render::draw_map(window, constraint, map);
			render::draw_grid(window, constraint);
		window.display();
	}
}