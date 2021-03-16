#include "Nova3DEngine.h"

using namespace nova;
using namespace std;

NovaEngine::NovaEngine()
{

}

NovaEngine::~NovaEngine()
{
	delete[] depth_buffer_;
}

bool NovaEngine::Setup(const int width, const int height, const int pixel_scale, bool fullscreen)
{
	window_width_ = width;
	window_height_ = height;
	pixel_scale_ = pixel_scale;
	width_ = window_width_ / pixel_scale_;
	height_ = window_height_ / pixel_scale_;
	depth_buffer_ = new float[width * height];
	memset(&depth_buffer_[0], FLT_MAX, width_ * height_ * sizeof(float));

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
	auto framerate_time_delta = sf::Time::Zero;
	
	// create pixel buffer
	auto *pixels = new Texture(width_, height_);

	sf::Image pixel_buffer;
	pixel_buffer.create(width_, height_);

	sf::Texture texture;
	texture.create(width_, height_);

	auto buffer = sf::Sprite(texture);
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
	clear.setSize({ static_cast<float>(width_), static_cast<float>(height_) });

	unsigned int framerate = 0;
	while (render_window_->isOpen()) 
	{
		// process input
		sf::Event event{};
		while (render_window_->pollEvent(event)) 
			this->ProcessInput(event);
		
		// check if the user closed the window
		if (!is_running_)
		{
			render_window_->close();
			break;
		}

        auto delta = clock.restart();
		framerate_time_delta += delta;
		
		if (framerate_time_delta >= sf::seconds(1))
		{
			framerate_time_delta = sf::Time::Zero;
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
		memset(depth_buffer_, FLT_MAX, width_ * height_ * sizeof(float));		
		for (auto x = 0; x < width_; x++)
		{
			for (auto y = 0; y < height_; y++)
				pixels->SetPixel(x, y, fog_colour_);
		}

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
		lines.emplace_back(sf::Vertex(sf::Vector2f(0, 0)));
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 4.f, 0)));
		lines.emplace_back(sf::Vertex(sf::Vector2f(0, height_ / 4.f - 1)));
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 4.f, height_ / 4.f - 1)));

		lines.emplace_back(sf::Vertex(sf::Vector2f(1, 0)));
		lines.emplace_back(sf::Vertex(sf::Vector2f(1, height_ / 4.f - 1)));
		
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 4.f, 0)));
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 4.f, height_ / 4.f - 1)));

		// player direction
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f)));
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f - 10)));

		// fov
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f), sf::Color(255, 255, 255, 20)));
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f - cosf(camera_->GetFOVHalf() + Math::half_pi_) * 300, height_ / 4.f - sinf(camera_->GetFOVHalf() + Math::half_pi_) * 300), sf::Color(255, 255, 255, 20)));
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f), sf::Color(255, 255, 255, 20)));	
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f + cosf(camera_->GetFOVHalf() + Math::half_pi_) * 300, height_ / 4.f - sinf(camera_->GetFOVHalf() + Math::half_pi_) * 300), sf::Color(255, 255, 255, 20)));

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
	UserUnload();
}


void NovaEngine::RenderMap(const class Node& render_node, const class Node& last_node,
	const sf::Vector2f normalized_bounds[4], class Texture* pixels, class sf::RenderTexture& minimap)
{
	// render walls and minimap
	std::vector<sf::Vertex> lines;
	for (auto *wall : render_node.walls_)
	{
		if (wall->node_index_ == 100)
			continue;

		// transform wall relative to player
		auto tx1 = wall->p1_.x - camera_->GetPosition().x;
		auto ty1 = wall->p1_.y - camera_->GetPosition().y;
		auto tx2 = wall->p2_.x - camera_->GetPosition().x;
		auto ty2 = wall->p2_.y - camera_->GetPosition().y;

		// rotate points around the player's view
		auto tz1 = tx1 * camera_->GetCosAngle() + ty1 * camera_->GetSinAngle();
		auto tz2 = tx2 * camera_->GetCosAngle() + ty2 * camera_->GetSinAngle();
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
		auto wall_colour = (wall->IsPortal()) ? sf::Color(255, 0, 0, 25) : sf::Color(255, 255, 255, 50);
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f - points[0].xyz.x, height_ / 4.f - points[0].xyz.z), wall_colour));
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f - points[1].xyz.x, height_ / 4.f - points[1].xyz.z), wall_colour));

		// clip
		auto is_needing_to_render = true;
		for (const auto &plane : camera_->GetClippingPlanes()) 
		{
			// see if we need to clip
			const auto cross1 = plane.Distance(points[0].xyz);
			const auto cross2 = plane.Distance(points[1].xyz);

			// wall outside of plane, don't render
			if ((cross1 < 0) && (cross2 < 0))
			{
				is_needing_to_render = false;
				break;
			}

			if (cross1 < 0 || cross2 < 0)
			{		
				const auto intersect = plane.Intersect(points[0].xyz, points[1].xyz);
				auto u = Slope(points[1].uvw.x - points[0].uvw.x, points[1].xyz.x - points[0].xyz.x);
				auto x = Slope(points[1].original_position.x - points[0].original_position.x, points[1].xyz.x - points[0].xyz.x);
				auto y = Slope(points[1].original_position.y - points[0].original_position.y, points[1].xyz.z - points[0].xyz.z);
				if (cross1 < 0)
				{
					points[0].uvw.x = u.Interpolate(points[0].uvw.x, intersect.x - points[0].xyz.x);
					points[0].original_position.x = x.Interpolate(points[0].original_position.x, intersect.x - points[0].xyz.x);
					points[0].original_position.y = y.Interpolate(points[0].original_position.y, intersect.z - points[0].xyz.z);
					points[0].xyz.x = intersect.x;
					points[0].xyz.z = intersect.z;
				}
				else if (cross2 < 0)
				{
					points[1].uvw.x = u.Interpolate(points[0].uvw.x, intersect.x - points[0].xyz.x);
					points[1].original_position.x = x.Interpolate(points[0].original_position.x, intersect.x - points[0].xyz.x);
					points[1].original_position.y = y.Interpolate(points[0].original_position.y, intersect.z - points[0].xyz.z);
					points[1].xyz.x = intersect.x;
					points[1].xyz.z = intersect.z;
				}
			}
		}

		if (!is_needing_to_render)
			continue;

		points[3].xyz = points[0].xyz;
		points[2].xyz = points[1].xyz;

		// draw clipped walls on minimap
		wall_colour = (wall->IsPortal()) ? sf::Color(255, 0, 0, 50) : sf::Color::White;
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f - points[0].xyz.x, height_ / 4.f - points[0].xyz.z), wall_colour));
		lines.emplace_back(sf::Vertex(sf::Vector2f(width_ / 8.f - points[1].xyz.x, height_ / 4.f - points[1].xyz.z), wall_colour));

		// perspective transform and normalize
		if (wall->texture_height_pixels_ == 0)
			wall->texture_height_pixels_ = 1;

		auto scale = camera_->GetScale();
		auto floor = render_node.floor_height_ - camera_->GetPosition().z;
		auto ceiling = render_node.ceiling_height_ - camera_->GetPosition().z;
		auto half_height = 0.5f * height_;
		auto half_width = 0.5f * width_;

		points[3].uvw.y = (ceiling - floor) / wall->texture_height_pixels_;
		points[2].uvw.y = (ceiling - floor) / wall->texture_height_pixels_;

		// wall projection
		points[0].xyz.y = (half_height - ceiling * scale.y / points[0].xyz.z)  / height_;
		points[1].xyz.y = (half_height - ceiling * scale.y / points[1].xyz.z)  / height_;
		points[2].xyz.y = (half_height - floor *   scale.y / points[2].xyz.z)  / height_;
		points[3].xyz.y = (half_height - floor *   scale.y / points[3].xyz.z)  / height_;
					   
		points[0].xyz.x = (half_width - points[0].xyz.x * scale.x / points[0].xyz.z) / width_;
		points[1].xyz.x = (half_width - points[1].xyz.x * scale.x / points[1].xyz.z) / width_;
		points[2].xyz.x = (half_width - points[2].xyz.x * scale.x / points[2].xyz.z) / width_;
		points[3].xyz.x = (half_width - points[3].xyz.x * scale.x / points[3].xyz.z) / width_;

		// wall uv perspective correction
		for (auto &point : points)
        {
            point.uvw.x /= point.xyz.z;
            point.original_position.x /= point.xyz.z;
            point.original_position.y /= point.xyz.z;
		}

		points[0].uvw.z = 1.f / points[0].xyz.z;
		points[1].uvw.z = 1.f / points[1].xyz.z;

		int x1 = std::max(points[0].xyz.x * width_, normalized_bounds[0].x * width_);
		x1 = Math::Clamp(0, width_, x1);
		int x2 = std::min(points[1].xyz.x * width_, (normalized_bounds[1].x) * width_);
		x2 = Math::Clamp(0, width_, x2);

		float z_start_inv = 1 / points[0].xyz.z;
		float z_end_inv = 1 / points[1].xyz.z;

		// check if this is a portal
		float portal_y_boundaries[4] = { 0, 0, 0, 0 };
		if ((wall->IsPortal()) && (current_map_->nodes_[wall->node_index_] != &last_node))
		{
			auto &next_node = *current_map_->nodes_[wall->node_index_];
			auto next_node_ceiling = next_node.ceiling_height_ - camera_->GetPosition().z;
			auto next_node_floor = next_node.floor_height_ - camera_->GetPosition().z;
			float next_node_y[4] =
			{
				(half_height - next_node_ceiling * scale.y / points[0].xyz.z) / height_,
				(half_height - next_node_ceiling * scale.y / points[1].xyz.z) / height_,
				(half_height - next_node_floor * scale.y / points[2].xyz.z) / height_,
				(half_height - next_node_floor * scale.y / points[3].xyz.z) / height_
			};
					
			portal_y_boundaries[0] = std::max(next_node_y[0], points[0].xyz.y);
			portal_y_boundaries[1] = std::max(next_node_y[1], points[1].xyz.y);
			portal_y_boundaries[2] = std::min(next_node_y[2], points[2].xyz.y);
			portal_y_boundaries[3] = std::min(next_node_y[3], points[3].xyz.y);
			sf::Vector2f render_bounds[4] =
			{
				{ points[0].xyz.x, portal_y_boundaries[0] },
				{ points[1].xyz.x, portal_y_boundaries[1] },
				{ points[2].xyz.x, portal_y_boundaries[2] },
				{ points[3].xyz.x, portal_y_boundaries[3] }
			};

			RenderMap(next_node, render_node, render_bounds, pixels, minimap);
		}

		auto dx = points[1].xyz.x - points[0].xyz.x;

		auto ya = Slope(points[1].xyz.y - points[0].xyz.y, dx);
		auto yb = Slope(points[2].xyz.y - points[3].xyz.y, dx);
		auto z_slope = Slope(points[1].xyz.z - points[0].xyz.z, dx);
		
		auto y_top = Slope(normalized_bounds[1].y - normalized_bounds[0].y, normalized_bounds[1].x - normalized_bounds[0].x);
		auto y_bottom = Slope(normalized_bounds[2].y - normalized_bounds[3].y, normalized_bounds[1].x - normalized_bounds[0].x);
		
		auto w_slope = Slope(points[1].uvw.z - points[0].uvw.z, dx);
		auto u_slope = Slope(points[1].uvw.x - points[0].uvw.x, dx);
		
		auto x_map = Slope(points[1].original_position.x - points[0].original_position.x, x2 - x1);
		auto y_map = Slope(points[1].original_position.y - points[0].original_position.y, x2 - x1);

		for (auto x = x1; x < x2; x++) 
		{
			// normalize x
			auto nx = x / static_cast<float>(width_);

			// find y values
			auto dnx = nx - points[0].xyz.x;
			int y1 = ya.Interpolate(points[0].xyz.y, dnx) * height_;
			int y2 = yb.Interpolate(points[3].xyz.y, dnx) * height_;
			int y_min = y_top.Interpolate(normalized_bounds[0].y, nx - normalized_bounds[0].x) * height_;
			int y_max = y_bottom.Interpolate(normalized_bounds[3].y, nx - normalized_bounds[0].x) * height_;

			// perspective correct u
			auto z = z_slope.Interpolate(points[0].xyz.z, dnx);
			auto w = w_slope.Interpolate(points[0].uvw.z, dnx);
			auto u = u_slope.Interpolate(points[0].uvw.x, dnx) / w;

			auto uv = sf::FloatRect(u, 0, points[0].uvw.y, points[3].uvw.y);
			
			// fix wall tex coords
			auto v = Slope(uv.height - uv.top, static_cast<float>(y2 - y1));
			if (y_min > y1)
			{
				uv.top = v.Interpolate(uv.top, static_cast<float>(y_min - y1));
				y1 = y_min;
			}

			if (y_max < y2)
			{
				uv.height = v.Interpolate(uv.top, static_cast<float>(y_max - y1));
				y2 = y_max;
			}

			// figure out where we should be drawing
			sf::IntRect wall_screen_space = { x, y1, 1, y2 - y1 };

			// interpolate portal y boundaries
			int portal_y_min = (portal_y_boundaries[0] + ((portal_y_boundaries[1] - portal_y_boundaries[0]) / (points[1].xyz.x - points[0].xyz.x)) * (nx - points[0].xyz.x)) * height_;
			int portal_y_max = (portal_y_boundaries[3] + ((portal_y_boundaries[2] - portal_y_boundaries[3]) / (points[1].xyz.x - points[0].uvw.x)) * (nx - points[0].xyz.x)) * height_;
			if (portal_y_min < y_min)
				portal_y_min = y_min;

			if (portal_y_max > y_max)
				portal_y_max = y_max;

			auto x_origin = x_map.Interpolate(points[0].original_position.x, x - x1) / w;
			auto y_origin = y_map.Interpolate(points[0].original_position.y, x - x1) / w;

			// render wall			
			Point3D vertical_points[2] =
			{ 
				{ {static_cast<float>(x), static_cast<float>(y1), z}, {uv.left, uv.top, 1 }, { x_origin, y_origin, ceiling } },
				{ {static_cast<float>(x), static_cast<float>(y2) + 2, z}, {uv.width, uv.height, 1}, { x_origin, y_origin, floor } }
			};

			RasterizeVerticalSlice(
				pixels,
				*wall->wall_texture_,
				vertical_points,
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
	for (auto i = 0; i < static_cast<int>(render_node.plane_xy_.size()); i++) 
	{
		plane_polygon[i] =  
			{ 
				{ render_node.plane_xy_[i].x, render_node.plane_xy_[i].y, 0 },
				{ render_node.plane_uv_[i].x, render_node.plane_uv_[i].y, 0 },
				{ render_node.plane_xy_[i].x, render_node.plane_xy_[i].y, 0 },
			};
	}

	// translate and rotate
	for (auto i = 0; i < static_cast<int>(render_node.plane_xy_.size()); i++)
	{
		plane_polygon[i].xyz.x = plane_polygon[i].xyz.x - camera_->GetPosition().x;
		plane_polygon[i].xyz.y = plane_polygon[i].xyz.y - camera_->GetPosition().y;

		plane_polygon[i].xyz.z = plane_polygon[i].xyz.x * camera_->GetCosAngle() + plane_polygon[i].xyz.y * camera_->GetSinAngle();
		plane_polygon[i].xyz.x = plane_polygon[i].xyz.x * camera_->GetSinAngle() - plane_polygon[i].xyz.y * camera_->GetCosAngle();
	}

	// clip
	const auto vertices = ClipPolygon(&plane_polygon[0], static_cast<int>(render_node.plane_xy_.size()));
	if (vertices == 0)
		return;

	// draw floor on minimap
	sf::Vertex tri[6];
	for (auto i = 0; i < vertices - 3 + 1; i++) 
	{
		tri[0] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[0].xyz.x, height_ / 4.f - plane_polygon[0].xyz.z), sf::Color(255,255,0, 50));
		tri[1] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[1 + i].xyz.x, height_ / 4.f - plane_polygon[1 + i].xyz.z), sf::Color(255, 255, 0, 50));
													    
		tri[2] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[1 + i].xyz.x, height_ / 4.f - plane_polygon[1 + i].xyz.z), sf::Color(255, 255, 0, 50));
		tri[3] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[2 + i].xyz.x, height_ / 4.f - plane_polygon[2 + i].xyz.z), sf::Color(255, 255, 0, 50));
													    
		tri[4] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[2 + i].xyz.x, height_ / 4.f - plane_polygon[2 + i].xyz.z), sf::Color(255, 255, 0, 50));
		tri[5] = sf::Vertex(sf::Vector2f(width_ / 8.f - plane_polygon[0].xyz.x, height_ / 4.f - plane_polygon[0].xyz.z), sf::Color(255, 255, 0, 50));
		minimap.draw(tri, 6, sf::PrimitiveType::Lines);
	}

	const auto scale = camera_->GetScale();
	const auto floor = render_node.floor_height_ - camera_->GetPosition().z;
	const auto ceiling = render_node.ceiling_height_ - camera_->GetPosition().z;
	const auto half_height = 0.5f * height_;
	const auto half_width = 0.5f * width_;

	vector<float> ceiling_y;
	for (auto i = 0; i < vertices; i++)
	{
		// xyz
		plane_polygon[i].xyz.y = (half_height - floor * scale.y / plane_polygon[i].xyz.z) / height_;
		ceiling_y.push_back((half_height - ceiling * scale.y / plane_polygon[i].xyz.z) / height_);
		plane_polygon[i].xyz.x = (half_width - plane_polygon[i].xyz.x * scale.x / plane_polygon[i].xyz.z) / width_;
		plane_polygon[i].original_position.z = floor;

		// uvw
		plane_polygon[i].uvw.x /= plane_polygon[i].xyz.z;
		plane_polygon[i].uvw.y /= plane_polygon[i].xyz.z;
		plane_polygon[i].uvw.z = 1.f / plane_polygon[i].xyz.z;

		// original
		plane_polygon[i].original_position.x /= plane_polygon[i].xyz.z;
		plane_polygon[i].original_position.y /= plane_polygon[i].xyz.z;
	}

	// render floor
	if(vertices > 0)
		RasterizePolygon(pixels, &plane_polygon[0], vertices, *render_node.floor_texture_);

	// render ceiling
	for (auto i = 0; i < vertices; i++) 
	{
		plane_polygon[i].xyz.y = ceiling_y[i];
		plane_polygon[i].original_position.z = ceiling;
	}

	if (vertices > 0)
		RasterizePolygon(pixels, &plane_polygon[0], vertices, *render_node.ceiling_texture_);
}

// Rasterizes a CONVEX polygon with a CLOCKWISE winding
void NovaEngine::RasterizePolygon(class Texture* pixels, const struct Point3D vertices[], int vertex_count, const class Texture& texture)
{
	for (auto i = 0; i < vertex_count - 3 + 1; i++) 
	{
		Point3D points[3] =
		{
			vertices[0],
			vertices[1 + i],
			vertices[2 + i],
		};

		for (auto j = 0; j < 3; j++) 
		{
            auto swapped = false;
			for (auto k = 0; k < 3 - 1 - j; k++) 			
			{
				if (points[k].xyz.y < points[k + 1].xyz.y)
					continue;

				std::swap(points[k], points[k + 1]);
				swapped = true;
			}

			if (!swapped)
				break;
		}

		// rasterize
		int y1 = points[0].xyz.y * height_;
		int y2 = points[1].xyz.y * height_;
		int y3 = points[2].xyz.y * height_;
		
		auto ax = Slope(points[1].xyz.x - points[0].xyz.x, static_cast<float>(std::abs(y2 - y1)));
		auto ax_map = Slope(points[1].original_position.x - points[0].original_position.x, static_cast<float>(std::abs(y2 - y1)));
		auto ay_map = Slope(points[1].original_position.y - points[0].original_position.y, static_cast<float>(std::abs(y2 - y1)));
		auto au = Slope(points[1].uvw.x - points[0].uvw.x, static_cast<float>(std::abs(y2 - y1)));
		auto av = Slope(points[1].uvw.y - points[0].uvw.y, static_cast<float>(std::abs(y2 - y1)));
		auto aw = Slope(points[1].uvw.z - points[0].uvw.z, static_cast<float>(std::abs(y2 - y1)));
		
		auto bx = Slope(points[2].xyz.x - points[0].xyz.x, static_cast<float>(std::abs(y3 - y1)));
		auto bx_map = Slope(points[2].original_position.x - points[0].original_position.x, static_cast<float>(std::abs(y3 - y1)));
		auto by_map = Slope(points[2].original_position.y - points[0].original_position.y, static_cast<float>(std::abs(y3 - y1)));
		auto bu = Slope(points[2].uvw.x - points[0].uvw.x, static_cast<float>(std::abs(y3 - y1)));
		auto bv = Slope(points[2].uvw.y - points[0].uvw.y, static_cast<float>(std::abs(y3 - y1)));
		auto bw = Slope(points[2].uvw.z - points[0].uvw.z, static_cast<float>(std::abs(y3 - y1)));


		if (y2 - y1)
		{
            auto y_map = Slope(points[1].original_position.x - points[0].original_position.x, y2 - y1);
			for (int y = Math::Clamp(0, height_, y1); (y < y2) && (y < height_); y++)
			{
				int x1 = ax.Interpolate(points[0].xyz.x, y - y1) * width_;
				int x2 = bx.Interpolate(points[0].xyz.x, y - y1) * width_;
				
				auto u1 = au.Interpolate(points[0].uvw.x, y - y1);
				auto v1 = av.Interpolate(points[0].uvw.y, y - y1);
				auto w1 = aw.Interpolate(points[0].uvw.z, y - y1);
				auto x1_map = ax_map.Interpolate(points[0].original_position.x, y - y1);
				auto y1_map = ay_map.Interpolate(points[0].original_position.y, y - y1);
				
				auto u2 = bu.Interpolate(points[0].uvw.x, y - y1);
				auto v2 = bv.Interpolate(points[0].uvw.y, y - y1);
				auto w2 = bw.Interpolate(points[0].uvw.z, y - y1);
				auto x2_map = bx_map.Interpolate(points[0].original_position.x, y - y1);
				auto y2_map = by_map.Interpolate(points[0].original_position.y, y - y1);

				if (x1 > x2)
				{
					std::swap(x1, x2);
					std::swap(u1, u2);
					std::swap(v1, v2);
					std::swap(w1, w2);
					std::swap(x1_map, x2_map);
					std::swap(y1_map, y2_map);
				}

				auto tex_u = u1;
				auto tex_v = v1;
				auto tex_w = w1;
				auto x_map = x1_map;
				auto y_map = y1_map;
				
				auto tstep = 1.0f / ((float)(x2 - x1));
				auto t = 0.0f;
				
				for (int x = Math::Clamp(0, width_, x1); (x < x2) && (x < width_); x++)
				{					
					tex_u = (1.0f - t) * u1 + t * u2;
					tex_v = (1.0f - t) * v1 + t * v2;
					tex_w = (1.0f - t) * w1 + t * w2;
					x_map = (1.0f - t) * x1_map + t * x2_map;
					y_map = (1.0f - t) * y1_map + t * y2_map;

					auto z = 1.f / tex_w;
					if (depth_buffer_[x + y * width_] > tex_w)
						continue;

					depth_buffer_[x + y * width_] = tex_w;
					DrawPixel(
						pixels, x, y,
						tex_u / tex_w, tex_v / tex_w, texture,
						z,
						x_map / tex_w,
						y_map / tex_w,
						points[0].original_position.z);

					t += tstep;
				}
			}
		}

		if (y3 - y2)
		{
			ax = Slope(points[2].xyz.x - points[1].xyz.x, static_cast<float>(std::abs(y3 - y2)));
			ax_map = Slope(points[2].original_position.x - points[1].original_position.x, static_cast<float>(std::abs(y3 - y2)));
			ay_map = Slope(points[2].original_position.y - points[1].original_position.y, static_cast<float>(std::abs(y3 - y2)));
			au = Slope(points[2].uvw.x - points[1].uvw.x, static_cast<float>(std::abs(y3 - y2)));
			av = Slope(points[2].uvw.y - points[1].uvw.y, static_cast<float>(std::abs(y3 - y2)));
			aw = Slope(points[2].uvw.z - points[1].uvw.z, static_cast<float>(std::abs(y3 - y2)));

			bx = Slope(points[2].xyz.x - points[0].xyz.x, static_cast<float>(std::abs(y3 - y1)));
			for (int y = Math::Clamp(0, height_, y2); (y < y3) && (y < height_); y++)
			{
				int x1 = ax.Interpolate(points[1].xyz.x, y - y2) * width_;
				int x2 = bx.Interpolate(points[0].xyz.x, y - y1) * width_;

				auto u1 = au.Interpolate(points[1].uvw.x, y - y2);
				auto v1 = av.Interpolate(points[1].uvw.y, y - y2);
				auto w1 = aw.Interpolate(points[1].uvw.z, y - y2);
				auto x1_map = ax_map.Interpolate(points[1].original_position.x, y - y2);
				auto y1_map = ay_map.Interpolate(points[1].original_position.y, y - y2);
				
				auto u2 = bu.Interpolate(points[0].uvw.x, y - y1);
				auto v2 = bv.Interpolate(points[0].uvw.y, y - y1);
				auto w2 = bw.Interpolate(points[0].uvw.z, y - y1);
				auto x2_map = bx_map.Interpolate(points[0].original_position.x, y - y1);
				auto y2_map = by_map.Interpolate(points[0].original_position.y, y - y1);

				if (x1 > x2)
				{
					std::swap(x1, x2);
					std::swap(u1, u2);
					std::swap(v1, v2);
					std::swap(w1, w2);
					std::swap(x1_map, x2_map);
					std::swap(y1_map, y2_map);
				}

				auto tex_u = u1;
				auto tex_v = v1;
				auto tex_w = w1;
				auto x_map = x1_map;
				auto y_map = y1_map;
				
				auto tstep = 1.f / static_cast<float>(x2 - x1);
				auto t = 0.f;
				for (int x = Math::Clamp(0, width_, x1); (x < x2) && (x < width_); x++)
				{
					tex_u = (1.0f - t) * u1 + t * u2;
					tex_v = (1.0f - t) * v1 + t * v2;
					tex_w = (1.0f - t) * w1 + t * w2; 
					x_map = (1.0f - t) * x1_map + t * x2_map;
					y_map = (1.0f - t) * y1_map + t * y2_map;

					auto z = 1.f / tex_w;
					if (depth_buffer_[x + y * width_] > tex_w)
						continue;

					depth_buffer_[x + y * width_] = tex_w;
					
					DrawPixel(
						pixels, x, y,
						tex_u / tex_w, tex_v / tex_w, texture,
						z,	
						x_map / tex_w,
						y_map / tex_w,
						points[0].original_position.z);

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
	if (points[0].xyz.y == INT_MIN || points[1].xyz.y == INT_MIN)
		return;

	const int x = points[0].xyz.x;
	const auto u = points[0].uvw.x;
	const int y1 = Math::Clamp(0, height_, points[0].xyz.y);
	const int y2 = Math::Clamp(0, height_, points[1].xyz.y);

	auto v_slope = Slope(points[1].uvw.y - points[0].uvw.y, points[1].xyz.y - points[0].xyz.y);
	auto z_map = Slope(points[1].original_position.z - points[0].original_position.z, points[1].xyz.y - points[0].xyz.y);
	for (auto y = y1; y < y2; y++)
	{
		if ((y >= portal_screen_space.top) && (y <= (portal_screen_space.top + portal_screen_space.height)))
			y = portal_screen_space.top + portal_screen_space.height + 1;

		if ((y < y1) || (y >= y2))
			break;

		if (depth_buffer_[x + y * width_] == FLT_MAX)
			depth_buffer_[x + y * width_] = points[0].xyz.z;

		const auto v = v_slope.Interpolate(points[0].uvw.y, y - points[0].xyz.y);
		DrawPixel(
			pixels, x, y,
			u, v, texture,
			points[0].xyz.z,
			points[0].original_position.x,
			points[0].original_position.y,
			z_map.Interpolate(points[0].original_position.z, y - points[0].xyz.y));
	}
}

int NovaEngine::ClipPolygon(struct Point3D points[], const int num_points)
{
	std::vector<Point3D> new_points;
	new_points.resize(num_points * 2);
	auto poly_count = num_points;
	auto clipped_count = 0;

	for (const auto &plane : camera_->GetClippingPlanes()) 
	{
		clipped_count = 0;
		for (int i = 0; i < poly_count; i++)
		{
			const auto current = points[i];
			const auto next = points[(i + 1) % poly_count];

			const auto cross1 = plane.Distance(current.xyz);
			const auto cross2 = plane.Distance(next.xyz);

			if (cross1 >= 0 && cross2 >= 0)
			{
				new_points[clipped_count++] = next;
				continue;
			}
			else if (cross1 < 0 && cross2 >= 0) 
			{
				const auto intersect = plane.Intersect(current.xyz, next.xyz);

				auto u = Slope(next.uvw.x - current.uvw.x, next.xyz.x - current.xyz.x);
				auto v = Slope(next.uvw.y - current.uvw.y, next.xyz.z - current.xyz.z);
				auto x = Slope(next.original_position.x - current.original_position.x, next.xyz.x - current.xyz.x);
				auto y = Slope(next.original_position.y - current.original_position.y, next.xyz.z - current.xyz.z);

				Point3D intersecting_point;
				intersecting_point.uvw.x = u.Interpolate(current.uvw.x, intersect.x - current.xyz.x);
				intersecting_point.uvw.y = v.Interpolate(current.uvw.y, intersect.z - current.xyz.z);
				intersecting_point.original_position.x = x.Interpolate(current.original_position.x, intersect.x - current.xyz.x);
				intersecting_point.original_position.y = y.Interpolate(current.original_position.y, intersect.z - current.xyz.z);
				intersecting_point.xyz.x = intersect.x;
				intersecting_point.xyz.z = intersect.z;
				intersecting_point.xyz.y = current.xyz.y;

				new_points[clipped_count++] = intersecting_point;
				new_points[clipped_count++] = next;
			}
			else if (cross1 >= 0 && cross2 < 0) 
			{
				const auto intersect = plane.Intersect(current.xyz, next.xyz);

				auto u = Slope(next.uvw.x - current.uvw.x, next.xyz.x - current.xyz.x);
				auto v = Slope(next.uvw.y - current.uvw.y, next.xyz.z - current.xyz.z);
				auto x = Slope(next.original_position.x - current.original_position.x, next.xyz.x - current.xyz.x);
				auto y = Slope(next.original_position.y - current.original_position.y, next.xyz.z - current.xyz.z);

				Point3D intersecting_point;
				intersecting_point.uvw.x = u.Interpolate(current.uvw.x, intersect.x - current.xyz.x);
				intersecting_point.uvw.y = v.Interpolate(current.uvw.y, intersect.z - current.xyz.z);
				intersecting_point.original_position.x = x.Interpolate(current.original_position.x, intersect.x - current.xyz.x);
				intersecting_point.original_position.y = y.Interpolate(current.original_position.y, intersect.z - current.xyz.z);
				intersecting_point.xyz.x = intersect.x;
				intersecting_point.xyz.z = intersect.z;
				intersecting_point.xyz.y = current.xyz.y;

				new_points[clipped_count++] = intersecting_point;
			}
		}

		poly_count = clipped_count;
		for (auto i = 0; i < poly_count; i++)
			points[i] = new_points[i];
	}

	//delete[] new_points;
	return clipped_count;
}

inline void NovaEngine::DrawPixel(
    class Texture* pixels, const int x, const int y,
    float u, float v, const class Texture& texture,
    float z, const float map_x, const float map_y, const float map_z)
{	
	// quick lerp
	auto mix = [](float f, auto a, auto b)
	{
		return a + (b - a) * f;
	};

	// get texture pixel and bi-linear filter
	u = Math::Clamp(0, texture.GetWidth(), fmodf(u * texture.GetWidth(), texture.GetWidth()));
	v = Math::Clamp(0, texture.GetHeight(), fmodf(v * texture.GetHeight(), texture.GetHeight()));

	auto colour = texture.GetPixel(u, v);

	const auto dx = camera_->GetPosition().x - map_x;
	const auto dy = camera_->GetPosition().y - map_y;
	const auto dz = camera_->GetPosition().z - map_z;
	z = std::sqrtf(dx * dx + dy * dy + dz * dz);
	
	// calculate lights
	float r = 0;
	float g = 0;
	float b = 0;
	for (auto *light : lights_)
	{
		const auto dist = light->GetDistance(map_x, map_y, map_z);
		const auto light_magnitude = std::powf(dist, -2) * 1000;

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
	const auto light_lum = (0.2126f * r + 0.7152f * g + 0.0722f * b);

	// subtract light luminosity
	auto mag = std::powf(z, fog_density_) / 255.f - light_lum;
	if (mag < 0)
		mag = 0;

	if (mag >= 1)
	{
		colour = fog_colour_;
	}
	else
	{
		colour.r = mix(mag, colour.r, fog_colour_.r);
		colour.g = mix(mag, colour.g, fog_colour_.g);
		colour.b = mix(mag, colour.b, fog_colour_.b);
	}

	pixels->SetPixel(x, y, colour);
}

void NovaEngine::RenderNodeActors(const class Node& node, class Texture* pixels, const sf::FloatRect& normalized_bounds)
{
	for (auto actor : node.actors_) 
	{
	
	}
}

void NovaEngine::UpdateActorPositions()
{
	// portal traversal
	for (auto *wall : camera_->GetCurrentNode()->walls_) 
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