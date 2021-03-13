#include "Nova3DEngine.h"

using namespace nova;
using namespace std;

NovaEngine::NovaEngine()
{

}

NovaEngine::~NovaEngine()
{
	delete[] depth_buffer;
}

bool NovaEngine::Setup(int width, int height, int pixel_scale, bool fullscreen)
{
	window_width_ = width;
	window_height_ = height;
	pixel_scale_ = pixel_scale;
	width_ = window_width_ / pixel_scale_;
	height_ = window_height_ / pixel_scale_;
	depth_buffer = new float[width * height];
	memset(&depth_buffer[0], FLT_MAX, width_ * height_ * sizeof(float));

	input_manager_ = new UserInputManager();
	render_window_ = new sf::RenderWindow();
	render_window_->create(sf::VideoMode(window_width_, window_height_), "");

	render_window_->setFramerateLimit(1000);
	//render_window_->setVerticalSyncEnabled(true);
	camera_ = new Camera(width_, height_);
	ui_root_ = nullptr;
	fog_colour_ = sf::Color(0x11, 0x11, 0x22);
	fog_occlusion_distance_ = 150;
	fog_density_ = logf(255) / logf(fog_occlusion_distance_);
		
	
	Light* l1 = new Light(sf::Color::Magenta, { 170, 70, 10 });
	Light* l2 = new Light(sf::Color::Red, { 340, 70, 10 });
	Light* l3 = new Light(sf::Color::Green, { 340, 210, 10 });
	Light* l4 = new Light(sf::Color::Blue, { 170, 210, 10 });
	lights_.push_back(l1);
	lights_.push_back(l2);
	lights_.push_back(l3);
	lights_.push_back(l4);

	UserLoad();
	if (!camera_->GetCurrentNode()) 
	{		
		std::cout << "FAIL: CAMERA NODE NOT SET UP. Need to call GetCamera().SetCurrentNode(Node*)";
		return false;
	}


	return true;
}

void NovaEngine::Run() 
{
	is_running_ = true;
	sf::Clock clock;
	sf::Time timeSinceLastFramerateUpdate = sf::Time::Zero;
	
	// create pixel buffer
	Texture* pixels = new Texture(width_, height_);

	sf::Image pixel_buffer;
	pixel_buffer.create(width_, height_);

	sf::Texture texture;
	texture.create(width_, height_);

	sf::Sprite buffer = sf::Sprite(texture);
	buffer.setScale(pixel_scale_, pixel_scale_);
	
	// create minimap
	sf::RenderTexture minimap;
	minimap.create(width_ / 4, height_ / 4);
	sf::Sprite minimap_drawable;
	minimap_drawable.setScale(pixel_scale_, pixel_scale_);

	// create ui
	sf::RenderTexture ui;
	ui.create(width_, height_);
	sf::Sprite ui_drawable;
	ui_drawable.setScale(pixel_scale_, pixel_scale_);

	sf::RectangleShape clear;
	clear.setFillColor(fog_colour_);
	clear.setSize({ (float)width_, (float)height_ });

	unsigned int framerate = 0;
	/*for (auto light : lights_)
		translated_lights_.push_back(new Light(light->colour_, light->position_));*/

	while (render_window_->isOpen()) 
	{
		// process input
		sf::Event event;
		while (render_window_->pollEvent(event)) 
			this->ProcessInput(event);
		
		// check if the user closed the window
		if (!is_running_)
		{
			render_window_->close();
			break;
		}

		sf::Time delta = clock.restart();
		timeSinceLastFramerateUpdate += delta;
		
		if (timeSinceLastFramerateUpdate >= sf::seconds(1))
		{
			timeSinceLastFramerateUpdate = sf::Time::Zero;
			render_window_->setTitle(std::to_string(framerate));
			framerate = 0;
		}

		framerate++;		

		// ask user to update
		UpdateWorker(delta, *input_manager_);

		// check if the user wants to exit the game through game controls
		if (!is_running_)
		{
			render_window_->close();
			break;
		}

		// check for collision and update position of actors
		UpdateActorPositions();

		// render

		// clear
		ui.clear(sf::Color::Transparent);
		minimap.clear(sf::Color(0, 0, 0, 200));
		render_window_->draw(clear);
		memset(depth_buffer, FLT_MAX, width_ * height_ * sizeof(float));		
		for (int x = 0; x < width_; x++)
		{
			for (int y = 0; y < height_; y++)
				pixels->SetPixel(x, y, fog_colour_);
		}
		
		// translate lights
		/*for (int i = 0; i < lights_.size(); i++) 
		{
			translated_lights_[i]->position_.x = lights_[i]->position_.x - camera_->GetPosition().x;
			translated_lights_[i]->position_.y = lights_[i]->position_.y - camera_->GetPosition().y;
			translated_lights_[i]->position_.z = lights_[i]->position_.z - camera_->GetPosition().z;

			translated_lights_[i]->position_.z = translated_lights_[i]->position_.x * camera_->GetCosAngle() + translated_lights_[i]->position_.y * camera_->GetSinAngle();
			translated_lights_[i]->position_.x = translated_lights_[i]->position_.x * camera_->GetSinAngle() - plane_polygon[i].xyz_.y * camera_->GetCosAngle();
		}*/

		// render floors and ceiling
		for (auto* node : current_map_->nodes_)		
			RenderPlanes(pixels, *node, minimap);
		
		// render map
		sf::Vector2f render_bounds[4] = { {0, 0}, {1.f, 0}, {1.f, 1.f}, {0.f, 1.f} };
		RenderMap(*camera_->GetCurrentNode(), *camera_->GetCurrentNode(), render_bounds, pixels, minimap);

		// add to minimap
		sf::RectangleShape player;
		player.setPosition(width_ / 8.f - 3, height_ / 4.f - 3);
		player.setSize(sf::Vector2f(5, 5));

		// map borders
		std::vector<sf::Vertex> lines;
		lines.push_back(sf::Vertex(sf::Vector2f(0, 0)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 4.f, 0)));
		lines.push_back(sf::Vertex(sf::Vector2f(0, height_ / 4.f - 1)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 4.f, height_ / 4.f - 1)));

		lines.push_back(sf::Vertex(sf::Vector2f(1, 0)));
		lines.push_back(sf::Vertex(sf::Vector2f(1, height_ / 4.f - 1)));
		
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 4.f, 0)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 4.f, height_ / 4.f - 1)));

		// player direction
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f - 10)));

		// fov
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f), sf::Color(255, 255, 255, 20)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - cosf(camera_->GetFOVHalf() + Math::half_pi_) * 300, height_ / 4.f - sinf(camera_->GetFOVHalf() + Math::half_pi_) * 300), sf::Color(255, 255, 255, 20)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f), sf::Color(255, 255, 255, 20)));	
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f + cosf(camera_->GetFOVHalf() + Math::half_pi_) * 300, height_ / 4.f - sinf(camera_->GetFOVHalf() + Math::half_pi_) * 300), sf::Color(255, 255, 255, 20)));

		minimap.draw(&lines[0], lines.size(), sf::Lines);
		minimap.draw(player);

		// draw map
		texture.update(pixels->GetData());
		render_window_->draw(buffer);

		// draw minimap
		minimap.display();
		minimap_drawable.setTexture(minimap.getTexture());
		render_window_->draw(minimap_drawable);

		// draw ui
		RenderUI();
		ui.display();
		ui_drawable.setTexture(ui.getTexture());
		render_window_->draw(ui_drawable);

		render_window_->display();
	}

	delete pixels;
}

void NovaEngine::End() 
{
	is_running_ = false;
	for (int i = 0; i < lights_.size(); i++)
		delete lights_[i];

	UserUnload();
}


void NovaEngine::RenderMap(const class Node& render_node, const class Node& last_node,
	const sf::Vector2f normalized_bounds[4], class Texture* pixels, class sf::RenderTexture& minimap)
{
	// render walls and minimap
	std::vector<sf::Vertex> lines;
	for (auto wall : render_node.walls_)
	{
		if (wall->node_index_ == 100)
			continue;

		// transform wall relative to player
		float tx1 = wall->p1_.x - camera_->GetPosition().x;
		float ty1 = wall->p1_.y - camera_->GetPosition().y;
		float tx2 = wall->p2_.x - camera_->GetPosition().x;
		float ty2 = wall->p2_.y - camera_->GetPosition().y;

		// rotate points around the player's view
		float tz1 = tx1 * camera_->GetCosAngle() + ty1 * camera_->GetSinAngle();
		float tz2 = tx2 * camera_->GetCosAngle() + ty2 * camera_->GetSinAngle();
		tx1 = tx1 * camera_->GetSinAngle() - ty1 * camera_->GetCosAngle();
		tx2 = tx2 * camera_->GetSinAngle() - ty2 * camera_->GetCosAngle();

		// if wall fully behind us, don't render
		if (tz1 <= 0 && tz2 <= 0)
			continue;	

		// wall facing the wrong way
		/*if (tx1 > tx2)
			continue;*/

		Point3D points[4] = 
		{
			{ {tx1, ty1, tz1},	{ wall->uv1_.x, wall->uv1_.y, 1.f },	{ wall->p1_.x, wall->p1_.y, 0 } },
			{ {tx2, ty2, tz2},	{ wall->uv2_.x, wall->uv2_.y, 1.f },	{ wall->p2_.x, wall->p2_.y, 0 } },
			{ {0, 0, 0},		{ wall->uv2_.x, wall->uv2_.y, 1.f },	{ wall->p2_.x, wall->p2_.y, 0 } },
			{ {0, 0, 0},		{ wall->uv1_.x, wall->uv1_.y, 1.f },	{ wall->p1_.x, wall->p1_.y, 0 } },
		};

		// draw potentially out of frustum walls on minimap
		sf::Color wall_colour = (wall->IsPortal()) ? sf::Color(255, 0, 0, 25) : sf::Color(255, 255, 255, 50);
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - points[0].xyz_.x, height_ / 4.f - points[0].xyz_.z), wall_colour));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - points[1].xyz_.x, height_ / 4.f - points[1].xyz_.z), wall_colour));

		// clip
		bool is_needing_to_render = true;
		for (auto &plane : camera_->GetClippingPlanes()) 
		{
			// see if we need to clip
			float cross1 = plane.Distance(points[0].xyz_);
			float cross2 = plane.Distance(points[1].xyz_);

			// wall outside of plane, don't render
			if ((cross1 < 0) && (cross2 < 0))
			{
				is_needing_to_render = false;
				break;
			}

			if (cross1 < 0 || cross2 < 0)
			{		
				sf::Vector3f intersect = plane.Intersect(points[0].xyz_, points[1].xyz_);
				Slope u = Slope(points[1].uvw_.x - points[0].uvw_.x, points[1].xyz_.x - points[0].xyz_.x);
				Slope x = Slope(points[1].original_position_.x - points[0].original_position_.x, points[1].xyz_.x - points[0].xyz_.x);
				Slope y = Slope(points[1].original_position_.y - points[0].original_position_.y, points[1].xyz_.z - points[0].xyz_.z);
				if (cross1 < 0)
				{
					points[0].uvw_.x = u.Interpolate(points[0].uvw_.x, intersect.x - points[0].xyz_.x);
					points[0].original_position_.x = x.Interpolate(points[0].original_position_.x, intersect.x - points[0].xyz_.x);
					points[0].original_position_.y = y.Interpolate(points[0].original_position_.y, intersect.z - points[0].xyz_.z);
					points[0].xyz_.x = intersect.x;
					points[0].xyz_.z = intersect.z;
				}
				else if (cross2 < 0)
				{
					points[1].uvw_.x = u.Interpolate(points[0].uvw_.x, intersect.x - points[0].xyz_.x);
					points[1].original_position_.x = x.Interpolate(points[0].original_position_.x, intersect.x - points[0].xyz_.x);
					points[1].original_position_.y = y.Interpolate(points[0].original_position_.y, intersect.z - points[0].xyz_.z);
					points[1].xyz_.x = intersect.x;
					points[1].xyz_.z = intersect.z;
				}
			}
		}

		if (!is_needing_to_render)
			continue;

		points[3].xyz_ = points[0].xyz_;
		points[2].xyz_ = points[1].xyz_;

		// draw clipped walls on minimap
		wall_colour = (wall->IsPortal()) ? sf::Color(255, 0, 0, 50) : sf::Color::White;
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - points[0].xyz_.x, height_ / 4.f - points[0].xyz_.z), wall_colour));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - points[1].xyz_.x, height_ / 4.f - points[1].xyz_.z), wall_colour));

		// perspective transform and normalize
		if (wall->texture_height_pixels_ == 0)
			wall->texture_height_pixels_ = 1;

		sf::Vector2f scale = camera_->GetScale();
		float floor = render_node.floor_height_ - camera_->GetPosition().z;
		float ceiling = render_node.ceiling_height_ - camera_->GetPosition().z;
		float half_height = 0.5f * height_;
		float half_width = 0.5f * width_;

		points[3].uvw_.y = (ceiling - floor) / wall->texture_height_pixels_;
		points[2].uvw_.y = (ceiling - floor) / wall->texture_height_pixels_;

		// wall projection
		points[0].xyz_.y = (half_height - ceiling * scale.y / points[0].xyz_.z)  / height_;
		points[1].xyz_.y = (half_height - ceiling * scale.y / points[1].xyz_.z)  / height_;
		points[2].xyz_.y = (half_height - floor *   scale.y / points[2].xyz_.z)  / height_;
		points[3].xyz_.y = (half_height - floor *   scale.y / points[3].xyz_.z)  / height_;
					   
		points[0].xyz_.x = (half_width - points[0].xyz_.x * scale.x / points[0].xyz_.z) / width_;
		points[1].xyz_.x = (half_width - points[1].xyz_.x * scale.x / points[1].xyz_.z) / width_;
		points[2].xyz_.x = (half_width - points[2].xyz_.x * scale.x / points[2].xyz_.z) / width_;
		points[3].xyz_.x = (half_width - points[3].xyz_.x * scale.x / points[3].xyz_.z) / width_;

		// wall uv perspective correction
		points[0].uvw_.x /= points[0].xyz_.z;
		points[1].uvw_.x /= points[1].xyz_.z;
		points[2].uvw_.x /= points[2].xyz_.z;
		points[3].uvw_.x /= points[3].xyz_.z;

		points[0].uvw_.z = 1.f / points[0].xyz_.z;
		points[1].uvw_.z = 1.f / points[1].xyz_.z;

		int x1 = std::max(points[0].xyz_.x * width_, normalized_bounds[0].x * width_);
		x1 = Math::Clamp(0, width_, x1);
		int x2 = std::min(points[1].xyz_.x * width_, (normalized_bounds[1].x) * width_);
		x2 = Math::Clamp(0, width_, x2);

		float z_start_inv = 1 / points[0].xyz_.z;
		float z_end_inv = 1 / points[1].xyz_.z;

		// check if this is a portal
		float portal_y_boundaries[4] = { 0, 0, 0, 0 };
		if ((wall->IsPortal()) && (current_map_->nodes_[wall->node_index_] != &last_node))
		{
			Node& next_node = *current_map_->nodes_[wall->node_index_];
			float next_node_ceiling = next_node.ceiling_height_ - camera_->GetPosition().z;
			float next_node_floor = next_node.floor_height_ - camera_->GetPosition().z;
			float next_node_y[4] =
			{
				(half_height - next_node_ceiling * scale.y / points[0].xyz_.z) / height_,
				(half_height - next_node_ceiling * scale.y / points[1].xyz_.z) / height_,
				(half_height - next_node_floor * scale.y / points[2].xyz_.z) / height_,
				(half_height - next_node_floor * scale.y / points[3].xyz_.z) / height_
			};
					
			portal_y_boundaries[0] = std::max(next_node_y[0], points[0].xyz_.y);
			portal_y_boundaries[1] = std::max(next_node_y[1], points[1].xyz_.y);
			portal_y_boundaries[2] = std::min(next_node_y[2], points[2].xyz_.y);
			portal_y_boundaries[3] = std::min(next_node_y[3], points[3].xyz_.y);
			sf::Vector2f render_bounds[4] =
			{
				{ points[0].xyz_.x, portal_y_boundaries[0] },
				{ points[1].xyz_.x, portal_y_boundaries[1] },
				{ points[2].xyz_.x, portal_y_boundaries[2] },
				{ points[3].xyz_.x, portal_y_boundaries[3] }
			};

			RenderMap(next_node, render_node, render_bounds, pixels, minimap);
		}

		float dx = points[1].xyz_.x - points[0].xyz_.x;

		Slope ya = Slope(points[1].xyz_.y - points[0].xyz_.y, dx);
		Slope yb = Slope(points[2].xyz_.y - points[3].xyz_.y, dx);
		Slope z_slope = Slope(points[1].xyz_.z - points[0].xyz_.z, dx);

		Slope y_top = Slope(normalized_bounds[1].y - normalized_bounds[0].y, normalized_bounds[1].x - normalized_bounds[0].x);
		Slope y_bottom = Slope(normalized_bounds[2].y - normalized_bounds[3].y, normalized_bounds[1].x - normalized_bounds[0].x);
		
		Slope w_slope = Slope(points[1].uvw_.z - points[0].uvw_.z, dx);
		Slope u_slope = Slope(points[1].uvw_.x - points[0].uvw_.x, dx);

		Slope x_map = Slope(points[1].original_position_.x - points[0].original_position_.x, x2 - x1);
		Slope y_map = Slope(points[1].original_position_.y - points[0].original_position_.y, x2 - x1);

		for (int x = x1; x < x2; x++) 
		{
			// normalize x
			float nx = x / (float)width_;

			// find y values
			float dnx = nx - points[0].xyz_.x;
			int y1 = ya.Interpolate(points[0].xyz_.y, dnx) * height_;
			int y2 = yb.Interpolate(points[3].xyz_.y, dnx) * height_;
			int y_min = y_top.Interpolate(normalized_bounds[0].y, nx - normalized_bounds[0].x) * height_;
			int y_max = y_bottom.Interpolate(normalized_bounds[3].y, nx - normalized_bounds[0].x) * height_;

			// perspective correct u
			float z = z_slope.Interpolate(points[0].xyz_.z, dnx);
			float w = w_slope.Interpolate(points[0].uvw_.z, dnx);
			float u = u_slope.Interpolate(points[0].uvw_.x, dnx) / w;

			sf::FloatRect uv = sf::FloatRect(u, 0, points[0].uvw_.y, points[3].uvw_.y);
			
			// fix wall tex coords
			Slope v = Slope(uv.height - uv.top, (float)(y2 - y1));
			if (y_min > y1)
			{
				uv.top = v.Interpolate(uv.top, (float)(y_min - y1));
				y1 = y_min;
			}

			if (y_max < y2)
			{
				uv.height = v.Interpolate(uv.top, (float)(y_max - y1));
				y2 = y_max;
			}

			// figure out where we should be drawing
			sf::IntRect wall_screen_space = { x, y1, 1, y2 - y1 };

			// interpolate portal y boundaries
			int portal_y_min = (portal_y_boundaries[0] + ((portal_y_boundaries[1] - portal_y_boundaries[0]) / (points[1].xyz_.x - points[0].xyz_.x)) * (nx - points[0].xyz_.x)) * height_;
			int portal_y_max = (portal_y_boundaries[3] + ((portal_y_boundaries[2] - portal_y_boundaries[3]) / (points[1].xyz_.x - points[0].uvw_.x)) * (nx - points[0].xyz_.x)) * height_;
			if (portal_y_min < y_min)
				portal_y_min = y_min;

			if (portal_y_max > y_max)
				portal_y_max = y_max;

			float x_origin = x_map.Interpolate(points[0].original_position_.x, x - x1);
			float y_origin = y_map.Interpolate(points[0].original_position_.y, x - x1);

			// render wall			
			Point3D points[2] =
			{ 
				{ {(float)x, (float)y1, z}, {uv.left, uv.top, 1 }, { x_origin, y_origin, ceiling } },
				{ {(float)x, (float)y2 + 2, z}, {uv.width, uv.height, 1}, { x_origin, y_origin, floor } }
			};

			RasterizeVerticalSlice(
				pixels,
				*wall->wall_texture_,
				points,
				{ 0, portal_y_min + 1, 0, portal_y_max - portal_y_min - 2 } );
		}
	}

	minimap.draw(&lines[0], lines.size(), sf::Lines);
}

void NovaEngine::RenderPlanes(class Texture* pixels, const class Node& render_node, class sf::RenderTexture& minimap)
{
	// render the floor/ceiling. this uses actual 3d projection!
	// floor
	vector<Point3D> plane_polygon;
	plane_polygon.resize(render_node.plane_xy_.size() * 2);
	for (int i = 0; i < render_node.plane_xy_.size(); i++) 
	{
		plane_polygon[i] =  
			{ 
				{ render_node.plane_xy_[i].x, render_node.plane_xy_[i].y, 0 },
				{ render_node.plane_uv_[i].x, render_node.plane_uv_[i].y, 0 },
				{ render_node.plane_xy_[i].x, render_node.plane_xy_[i].y, 0 },
			};
	}

	// translate and rotate
	for (int i = 0; i < render_node.plane_xy_.size(); i++)
	{
		plane_polygon[i].xyz_.x = plane_polygon[i].xyz_.x - camera_->GetPosition().x;
		plane_polygon[i].xyz_.y = plane_polygon[i].xyz_.y - camera_->GetPosition().y;

		plane_polygon[i].xyz_.z = plane_polygon[i].xyz_.x * camera_->GetCosAngle() + plane_polygon[i].xyz_.y * camera_->GetSinAngle();
		plane_polygon[i].xyz_.x = plane_polygon[i].xyz_.x * camera_->GetSinAngle() - plane_polygon[i].xyz_.y * camera_->GetCosAngle();
	}

	// clip
	int vertices = ClipPolygon(&plane_polygon[0], render_node.plane_xy_.size());
	if (vertices == 0)
		return;

	// draw floor on minimap
	sf::Vertex tri[6];
	for (int i = 0; i < vertices - 3 + 1; i++) 
	{
		tri[0] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[0].xyz_.x, height_ / 4.f - plane_polygon[0].xyz_.z), sf::Color(255,255,0, 50));
		tri[1] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[1 + i].xyz_.x, height_ / 4.f - plane_polygon[1 + i].xyz_.z), sf::Color(255, 255, 0, 50));
													    
		tri[2] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[1 + i].xyz_.x, height_ / 4.f - plane_polygon[1 + i].xyz_.z), sf::Color(255, 255, 0, 50));
		tri[3] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[2 + i].xyz_.x, height_ / 4.f - plane_polygon[2 + i].xyz_.z), sf::Color(255, 255, 0, 50));
													    
		tri[4] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[2 + i].xyz_.x, height_ / 4.f - plane_polygon[2 + i].xyz_.z), sf::Color(255, 255, 0, 50));
		tri[5] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[0].xyz_.x, height_ / 4.f - plane_polygon[0].xyz_.z), sf::Color(255, 255, 0, 50));
		minimap.draw(tri, 6, sf::PrimitiveType::Lines);
	}

	sf::Vector2f scale = camera_->GetScale();
	float floor = render_node.floor_height_ - camera_->GetPosition().z;
	float ceiling = render_node.ceiling_height_ - camera_->GetPosition().z;
	float half_height = 0.5f * height_;
	float half_width = 0.5f * width_;

	vector<float> ceiling_y;
	for (int i = 0; i < vertices; i++)
	{
		// xyz
		plane_polygon[i].xyz_.y = (half_height - floor * scale.y / plane_polygon[i].xyz_.z) / height_;
		ceiling_y.push_back((half_height - ceiling * scale.y / plane_polygon[i].xyz_.z) / height_);
		plane_polygon[i].xyz_.x = (half_width - plane_polygon[i].xyz_.x * scale.x / plane_polygon[i].xyz_.z) / width_;
		plane_polygon[i].original_position_.z = floor;

		// uvw
		plane_polygon[i].uvw_.x /= plane_polygon[i].xyz_.z;
		plane_polygon[i].uvw_.y /= plane_polygon[i].xyz_.z;
		plane_polygon[i].uvw_.z = 1.f / plane_polygon[i].xyz_.z;
	}

	// render floor
	if(vertices > 0)
		RasterizePolygon(pixels, &plane_polygon[0], vertices, *render_node.floor_texture_);

	// render ceiling
	for (int i = 0; i < vertices; i++) 
	{
		plane_polygon[i].xyz_.y = ceiling_y[i];
		plane_polygon[i].original_position_.z = ceiling;
	}

	if (vertices > 0)
		RasterizePolygon(pixels, &plane_polygon[0], vertices, *render_node.ceiling_texture_);
}

// Rasterizes a CONVEX polygon with a CLOCKWISE winding
void NovaEngine::RasterizePolygon(class Texture* pixels, const class Point3D vertices[], int vertex_count, const class Texture& texture)
{
	for (int i = 0; i < vertex_count - 3 + 1; i++) 
	{
		Point3D points[3] =
		{
			vertices[0],
			vertices[1 + i],
			vertices[2 + i],
		};

		for (int j = 0; j < 3; j++) 
		{
			bool swapped = false;
			for (int k = 0; k < 3 - 1 - j; k++) 			
			{
				if (points[k].xyz_.y < points[k + 1].xyz_.y)
					continue;

				std::swap(points[k], points[k + 1]);
				swapped = true;
			}

			if (!swapped)
				break;
		}

		// rasterize
		int y1 = points[0].xyz_.y * height_;
		int y2 = points[1].xyz_.y * height_;
		int y3 = points[2].xyz_.y * height_;
		
		Slope ax = Slope(points[1].xyz_.x - points[0].xyz_.x, (float)std::abs(y2 - y1));
		Slope au = Slope(points[1].uvw_.x - points[0].uvw_.x, (float)std::abs(y2 - y1));
		Slope av = Slope(points[1].uvw_.y - points[0].uvw_.y, (float)std::abs(y2 - y1));
		Slope aw = Slope(points[1].uvw_.z - points[0].uvw_.z, (float)std::abs(y2 - y1));

		Slope bx = Slope(points[2].xyz_.x - points[0].xyz_.x, (float)std::abs(y3 - y1));
		Slope bu = Slope(points[2].uvw_.x - points[0].uvw_.x, (float)std::abs(y3 - y1));
		Slope bv = Slope(points[2].uvw_.y - points[0].uvw_.y, (float)std::abs(y3 - y1));
		Slope bw = Slope(points[2].uvw_.z - points[0].uvw_.z, (float)std::abs(y3 - y1));
		
		Slope map_y = Slope(points[1].original_position_.y - points[0].original_position_.y, (float)std::abs(y2 - y1));

		if (y2 - y1)
		{
			for (int y = Math::Clamp(0, height_, y1); (y < y2) && (y < height_); y++)
			{
				int x1 = ax.Interpolate(points[0].xyz_.x, y - y1) * width_;
				int x2 = bx.Interpolate(points[0].xyz_.x, y - y1) * width_;
				
				float u1 = au.Interpolate(points[0].uvw_.x, y - y1);
				float v1 = av.Interpolate(points[0].uvw_.y, y - y1);
				float w1 = aw.Interpolate(points[0].uvw_.z, y - y1);

				float u2 = bu.Interpolate(points[0].uvw_.x, y - y1);
				float v2 = bv.Interpolate(points[0].uvw_.y, y - y1);
				float w2 = bw.Interpolate(points[0].uvw_.z, y - y1);

				if (x1 > x2)
				{
					std::swap(x1, x2);
					std::swap(u1, u2);
					std::swap(v1, v2);
					std::swap(w1, w2);
				}

				float tex_u = u1;
				float tex_v = v1;
				float tex_w = w1;

				float tstep = 1.0f / ((float)(x2 - x1));
				float t = 0.0f;

				Slope map_x = Slope(points[1].xyz_.x - points[0].original_position_.x, x2 - x1);
				float y_real = map_y.Interpolate(points[0].original_position_.y, y - y1);
				for (int x = Math::Clamp(0, width_, x1); (x < x2) && (x < width_); x++)
				{					
					tex_u = (1.0f - t) * u1 + t * u2;
					tex_v = (1.0f - t) * v1 + t * v2;
					tex_w = (1.0f - t) * w1 + t * w2;

					if (depth_buffer[x + y * width_] > tex_w)
						continue;

					depth_buffer[x + y * width_] = tex_w;
					DrawPixel(
						pixels, x, y,
						tex_u / tex_w, tex_v / tex_w, texture,
						1.f / tex_w,
						map_x.Interpolate(points[0].original_position_.x, x - x1),
						y_real,
						points[0].original_position_.y);

					t += tstep;
				}
			}
		}

		if (y3 - y2)
		{
			ax = Slope(points[2].xyz_.x - points[1].xyz_.x, (float)std::abs(y3 - y2));
			au = Slope(points[2].uvw_.x - points[1].uvw_.x, (float)std::abs(y3 - y2));
			av = Slope(points[2].uvw_.y - points[1].uvw_.y, (float)std::abs(y3 - y2));
			aw = Slope(points[2].uvw_.z - points[1].uvw_.z, (float)std::abs(y3 - y2));

			bx = Slope(points[2].xyz_.x - points[0].xyz_.x, (float)std::abs(y3 - y1));

			map_y = Slope(points[2].original_position_.y - points[1].original_position_.y, (float)std::abs(y3 - y2));
			for (int y = Math::Clamp(0, height_, y2); (y < y3) && (y < height_); y++)
			{
				int x1 = ax.Interpolate(points[1].xyz_.x, y - y2) * width_;
				int x2 = bx.Interpolate(points[0].xyz_.x, y - y1) * width_;

				float u1 = au.Interpolate(points[1].uvw_.x, y - y2);
				float v1 = av.Interpolate(points[1].uvw_.y, y - y2);
				float w1 = aw.Interpolate(points[1].uvw_.z, y - y2);

				float u2 = bu.Interpolate(points[0].uvw_.x, y - y1);
				float v2 = bv.Interpolate(points[0].uvw_.y, y - y1);
				float w2 = bw.Interpolate(points[0].uvw_.z, y - y1);

				if (x1 > x2)
				{
					std::swap(x1, x2);
					std::swap(u1, u2);
					std::swap(v1, v2);
					std::swap(w1, w2);
				}

				float tex_u = u1;
				float tex_v = v1;
				float tex_w = w1;

				float tstep = 1.0f / ((float)(x2 - x1));
				float t = 0.0f;

				Slope map_x = Slope(points[2].original_position_.x - points[1].original_position_.x, x2 - x1);
				float y_real = map_y.Interpolate(points[0].original_position_.y, y - y1);
				for (int x = Math::Clamp(0, width_, x1); (x < x2) && (x < width_); x++)
				{
					tex_u = (1.0f - t) * u1 + t * u2;
					tex_v = (1.0f - t) * v1 + t * v2;
					tex_w = (1.0f - t) * w1 + t * w2;

					if (depth_buffer[x + y * width_] > tex_w)
						continue;

					depth_buffer[x + y * width_] = tex_w;
					DrawPixel(
						pixels, x, y,
						tex_u / tex_w, tex_v / tex_w, texture,
						1.f / tex_w,	
						map_x.Interpolate(points[0].original_position_.x, x - x1),
						y_real,
						points[0].original_position_.z);

					t += tstep;
				}
			}
		}
	}
}

void NovaEngine::RasterizeVerticalSlice(
	class Texture* pixels, const class Texture& texture,
	const Point3D points[2], const sf::IntRect& portal_screen_space)
{
	if (points[0].xyz_.y == INT_MIN || points[1].xyz_.y == INT_MIN)
		return;

	int x = points[0].xyz_.x;
	float u = points[0].uvw_.x;
	int y1 = Math::Clamp(0, height_, points[0].xyz_.y);
	int y2 = Math::Clamp(0, height_, points[1].xyz_.y);

	Slope v_slope = Slope(points[1].uvw_.y - points[0].uvw_.y, points[1].xyz_.y - points[0].xyz_.y);
	Slope z_map = Slope(points[1].original_position_.z - points[0].original_position_.z, points[1].xyz_.y - points[0].xyz_.y);
	for (int y = y1; y < y2; y++)
	{
		if ((y >= portal_screen_space.top) && (y <= (portal_screen_space.top + portal_screen_space.height)))
			y = portal_screen_space.top + portal_screen_space.height + 1;

		if ((y < y1) || (y >= y2))
			break;

		if (depth_buffer[x + y * width_] == FLT_MAX)
			depth_buffer[x + y * width_] = points[0].xyz_.z;

		float v = v_slope.Interpolate(points[0].uvw_.y, y - points[0].xyz_.y);
		DrawPixel(
			pixels, x, y,
			u, v, texture,
			points[0].xyz_.z,
			points[0].original_position_.x,
			points[0].original_position_.y,
			z_map.Interpolate(points[0].original_position_.z, y - points[0].xyz_.y));
	}
}

int NovaEngine::ClipPolygon(class Point3D points[], int num_points)
{
	std::vector<Point3D> new_points;
	new_points.resize(num_points * 2);
	int poly_count = num_points;
	int clipped_count = 0;

	for (auto &plane : camera_->GetClippingPlanes()) 
	{
		clipped_count = 0;
		for (int i = 0; i < poly_count; i++)
		{
			Point3D current = points[i];
			Point3D next = points[(i + 1) % poly_count];

			float cross1 = plane.Distance(current.xyz_);
			float cross2 = plane.Distance(next.xyz_);

			if (cross1 >= 0 && cross2 >= 0)
			{
				new_points[clipped_count++] = next;
				continue;
			}
			else if (cross1 < 0 && cross2 >= 0) 
			{
				sf::Vector3f intersect = plane.Intersect(current.xyz_, next.xyz_);

				Slope u = Slope(next.uvw_.x - current.uvw_.x, next.xyz_.x - current.xyz_.x);
				Slope v = Slope(next.uvw_.y - current.uvw_.y, next.xyz_.z - current.xyz_.z);
				Slope x = Slope(next.original_position_.x - current.original_position_.x, next.xyz_.x - current.xyz_.x);
				Slope y = Slope(next.original_position_.y - current.original_position_.y, next.xyz_.z - current.xyz_.z);

				Point3D intersecting_point;
				intersecting_point.uvw_.x = u.Interpolate(current.uvw_.x, intersect.x - current.xyz_.x);
				intersecting_point.uvw_.y = v.Interpolate(current.uvw_.y, intersect.z - current.xyz_.z);
				intersecting_point.original_position_.x = x.Interpolate(current.original_position_.x, intersect.x - current.xyz_.x);
				intersecting_point.original_position_.y = y.Interpolate(current.original_position_.y, intersect.z - current.xyz_.z);
				intersecting_point.xyz_.x = intersect.x;
				intersecting_point.xyz_.z = intersect.z;
				intersecting_point.xyz_.y = current.xyz_.y;

				new_points[clipped_count++] = intersecting_point;
				new_points[clipped_count++] = next;
			}
			else if (cross1 >= 0 && cross2 < 0) 
			{
				sf::Vector3f intersect = plane.Intersect(current.xyz_, next.xyz_);

				Slope u = Slope(next.uvw_.x - current.uvw_.x, next.xyz_.x - current.xyz_.x);
				Slope v = Slope(next.uvw_.y - current.uvw_.y, next.xyz_.z - current.xyz_.z);
				Slope x = Slope(next.original_position_.x - current.original_position_.x, next.xyz_.x - current.xyz_.x);
				Slope y = Slope(next.original_position_.y - current.original_position_.y, next.xyz_.z - current.xyz_.z);

				Point3D intersecting_point;
				intersecting_point.uvw_.x = u.Interpolate(current.uvw_.x, intersect.x - current.xyz_.x);
				intersecting_point.uvw_.y = v.Interpolate(current.uvw_.y, intersect.z - current.xyz_.z);
				intersecting_point.original_position_.x = x.Interpolate(current.original_position_.x, intersect.x - current.xyz_.x);
				intersecting_point.original_position_.y = y.Interpolate(current.original_position_.y, intersect.z - current.xyz_.z);
				intersecting_point.xyz_.x = intersect.x;
				intersecting_point.xyz_.z = intersect.z;
				intersecting_point.xyz_.y = current.xyz_.y;

				new_points[clipped_count++] = intersecting_point;
			}
		}

		poly_count = clipped_count;
		for (int i = 0; i < poly_count; i++)
			points[i] = new_points[i];
	}

	//delete[] new_points;
	return clipped_count;
}

inline void NovaEngine::DrawPixel(
	class Texture* pixels, int x, int y,
	float u, float v, const class Texture& texture,
	float z, float map_x, float map_y, float map_z)
{	
	// quick lerp
	auto mix = [](float f, auto a, auto b)
	{
		return a + (b - a) * f;
	};

	// get texture pixel and bi-linear filter
	u = Math::Clamp(0, texture.GetWidth(), fmodf(u * texture.GetWidth(), texture.GetWidth()));
	v = Math::Clamp(0, texture.GetHeight(), fmodf(v * texture.GetHeight(), texture.GetHeight()));

	sf::Color colour = texture.GetPixel(u, v);

	float dx = camera_->GetPosition().x - map_x;
	float dy = camera_->GetPosition().y - map_y;
	float dz = camera_->GetPosition().z - map_z;
	z = std::sqrtf(dx * dx + dy * dy + dz * dz);
	
	// calculate lights
	float r = 0;
	float g = 0;
	float b = 0;
	for (auto *light : lights_)
	{
		float dist = light->GetDistance(map_x, map_y, map_z);
		float light_magnitude = std::powf(dist, -2) * 1000;

		r += light->colour_.r / 255.f * light_magnitude;
		g += light->colour_.g / 255.f * light_magnitude;
		b += light->colour_.b / 255.f * light_magnitude;
	}

	if (r > 1)
		r = 1;
	if (g > 1)
		g = 1;
	if (b > 1)
		b = 1;

	colour.r = mix(r, colour.r, 255);
	colour.g = mix(g, colour.g, 255);
	colour.b = mix(b, colour.b, 255);

	// mix fog
	/*float mag = std::powf(z, fog_density_) / 255.f;
	if (mag >= 1)
	{
		// mix light a little more here?
		colour = fog_colour_;
	}
	else
	{
		colour.r = mix(mag, colour.r, fog_colour_.r);
		colour.g = mix(mag, colour.g, fog_colour_.g);
		colour.b = mix(mag, colour.b, fog_colour_.b);
	}*/

	pixels->SetPixel(x, y, colour);
}

void NovaEngine::RenderNodeActors(const class Node& node, class Texture* pixels, const sf::FloatRect& normalized_bounds)
{
	float* depth_buffer = new float[width_ * height_];
	depth_buffer = { 0 };

	for (auto actor : node.actors_) 
	{
	
	}

	delete[] depth_buffer;
}

void NovaEngine::UpdateActorPositions()
{
	// portal traversal
	for (auto wall : camera_->GetCurrentNode()->walls_) 
	{
		if (!wall->IsPortal())
			continue;

		if (Math::CrossProduct(wall->p2_.x - wall->p1_.x, wall->p2_.y - wall->p1_.y, camera_->GetPosition().x - wall->p1_.x, camera_->GetPosition().y - wall->p1_.y) >= 0)
			continue;

		camera_->SetCurrentNode(current_map_->nodes_[wall->node_index_]);
		break;
	}
}

void NovaEngine::ProcessInput(const sf::Event& event)
{
	if (event.type == sf::Event::Closed) 
	{
		this->End();
		return;
	}

	if (event.type == sf::Event::MouseButtonPressed)
		input_manager_->mouse_buttons_[event.mouseButton.button] = true;
	if (event.type == sf::Event::MouseButtonReleased)
		input_manager_->mouse_buttons_[event.mouseButton.button] = false;
	if (event.type == sf::Event::KeyPressed)
		input_manager_->keys_[event.key.code] = true;
	if (event.type == sf::Event::KeyReleased)
		input_manager_->keys_[event.key.code] = false;
}

void NovaEngine::RenderUI()
{
	if (ui_root_ == nullptr)
		return;
}

IPlayer::IPlayer() :
	pitch_(0),
	node_(nullptr)
{
	this->position_ = sf::Vector3f();
	this->UpdateAngle(0);
}

IPlayer::IPlayer(const sf::Vector3f& position, float angle) :
	pitch_(0),
	node_(nullptr)
{
	this->position_ = position;
	this->UpdateAngle(angle);
}

IPlayer::~IPlayer()
{
}