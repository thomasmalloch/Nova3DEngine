#include "Nova3DEngine.h"

using namespace nova;
using namespace std;

NovaEngine::NovaEngine()
{

}

NovaEngine::~NovaEngine()
{
}

bool NovaEngine::Setup(int width, int height, int pixel_scale, bool fullscreen)
{
	window_width_ = width;
	window_height_ = height;
	pixel_scale_ = pixel_scale;
	width_ = window_width_ / pixel_scale_;
	height_ = window_height_ / pixel_scale_;

	input_manager_ = new UserInputManager();
	render_window_ = new sf::RenderWindow(sf::VideoMode(window_width_, window_height_), "Title");
	render_window_->setFramerateLimit(1000);
	//render_window_->setVerticalSyncEnabled(true);
	camera_ = new Camera(width_, height_, 0.1f, 2.0f);
	ui_root_ = nullptr;

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
	sf::Uint8* pixels = new sf::Uint8[width_ * height_ * 4];

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

	unsigned int framerate = 0;

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
		render_window_->clear(sf::Color::Black);		
		/*for (int x = 0; x < width_; x++)
			for (int y = 0; y <= height_ - 1; y++)
				pixels.setPixel(x, y, sf::Color::Black);*/

		// render map
		sf::Vector2f render_bounds[4] = { {0, 0}, {1.f, 0}, {1.f, 1.f}, {0.f, 1.f} };
		RenderMap(*camera_->GetCurrentNode(), *camera_->GetCurrentNode(), render_bounds, pixels, minimap);
		
		// add to minimap
		sf::RectangleShape player;
		player.setPosition(width_ / 8.f - 3, height_ / 4.f - 3);
		player.setSize(sf::Vector2f(5, 5));

		// map borders
		std::vector<sf::Vertex> lines;
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - 100, 0)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f + 100, 0)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - 100, height_ / 4.f - 1)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f + 100, height_ / 4.f - 1)));

		lines.push_back(sf::Vertex(sf::Vector2f(1, 0)));
		lines.push_back(sf::Vertex(sf::Vector2f(1, height_ / 4.f - 1)));
		
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 4.f, 0)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 4.f, height_ / 4.f - 1)));

		// player direction
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f - 10)));

		// fov
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f), sf::Color(255, 255, 255, 20)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - cosf(camera_->GetFOV() / 2) * 300, height_ / 4.f - sinf(camera_->GetFOV() / 2) * 300), sf::Color(255, 255, 255, 20)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f, height_ / 4.f), sf::Color(255, 255, 255, 20)));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f + cosf(camera_->GetFOV() / -2) * 300, height_ / 4.f + sinf(camera_->GetFOV() / -2) * 300), sf::Color(255, 255, 255, 20)));

		minimap.draw(&lines[0], lines.size(), sf::Lines);
		minimap.draw(player);

		// draw map
		texture.update(pixels);
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

	delete[] pixels;
}

inline void NovaEngine::End() 
{
	is_running_ = false;
	UserUnload();
}


void NovaEngine::RenderMap(const class Node& render_node, const class Node& last_node, const sf::Vector2f normalized_bounds[4], sf::Uint8 pixels[], class sf::RenderTexture& minimap)
{
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
		
		sf::Vector3f xyz_quad[4] =
		{
			{ tx1, ty1, tz1 },
			{ tx2, ty2, tz2 },
			{ 0, 0, 0 },
			{ 0, 0, 0 }
		};

		sf::Vector2f uv_quad[4] =
		{
			wall->uv1_,
			wall->uv2_,
			wall->uv1_,
			wall->uv2_
		};

		if ((xyz_quad[0].z) <= 0 && (xyz_quad[1].z <= 0))
			continue;

		// draw potentially out of frustum walls on minimap
		sf::Color wall_colour = (wall->IsPortal()) ? sf::Color(255, 0, 0, 25) : sf::Color(255, 255, 255, 50);
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - xyz_quad[0].x, height_ / 4.f - xyz_quad[0].z), wall_colour));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - xyz_quad[1].x, height_ / 4.f - xyz_quad[1].z), wall_colour));

		// clip
		const Frustum& f = camera_->GetWallClippingPlanes();
		bool is_needing_to_render = true;
		for (int i = 0; i < 2; i++) 
		{
			// see if we need to clip
			float cross1 = Math::CrossProduct(f.far_[i].x - f.near_[i].x, f.far_[i].y - f.near_[i].y, xyz_quad[0].x - f.near_[i].x, xyz_quad[0].z - f.near_[i].y);
			float cross2 = Math::CrossProduct(f.far_[i].x - f.near_[i].x, f.far_[i].y - f.near_[i].y, xyz_quad[1].x - f.near_[i].x, xyz_quad[1].z - f.near_[i].y);
			
			// wall outside of plane, don't render
			if ((cross1 >= 0) && (cross2 >= 0))
			{
				is_needing_to_render = false;
				break;
			}

			if (cross1 >= 0 || cross2 >= 0)
			{
				// fix plane
				sf::Vector2f intersect = Math::Intersect(f.far_[i].x, f.far_[i].y, f.near_[i].x, f.near_[i].y, xyz_quad[1].x, xyz_quad[1].z, xyz_quad[0].x, xyz_quad[0].z);
				if (cross1 >= 0)
				{
					uv_quad[0].x = uv_quad[0].x + ((uv_quad[1].x - uv_quad[0].x) / (xyz_quad[1].x - xyz_quad[0].x)) * (intersect.x - xyz_quad[0].x);
					xyz_quad[0].x = intersect.x;
					xyz_quad[0].z = intersect.y;
				}
				else if (cross2 >= 0)
				{
					uv_quad[1].x = uv_quad[0].x + ((uv_quad[1].x - uv_quad[0].x) / (xyz_quad[1].x - xyz_quad[0].x)) * (intersect.x - xyz_quad[0].x);
					xyz_quad[1].x = intersect.x;
					xyz_quad[1].z = intersect.y;
				}
			}
		}

		if (!is_needing_to_render)
			continue;

		xyz_quad[3] = xyz_quad[0];
		xyz_quad[2] = xyz_quad[1];

		// draw clipped walls on minimap
		wall_colour = (wall->IsPortal()) ? sf::Color(255, 0, 0, 50) : sf::Color::White;
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - xyz_quad[0].x, height_ / 4.f - xyz_quad[0].z), wall_colour));
		lines.push_back(sf::Vertex(sf::Vector2f(width_ / 8.f - xyz_quad[1].x, height_ / 4.f - xyz_quad[1].z), wall_colour));

		// perspective transform and normalize
		float floor = render_node.floor_height_ - camera_->GetPosition().z;
		float ceiling = render_node.ceiling_height_ - camera_->GetPosition().z;

		if (wall->texture_height_pixels_ == 0)
			wall->texture_height_pixels_ = 1;

		uv_quad[3].y = (ceiling - floor) / wall->texture_height_pixels_;
		uv_quad[2].y = (ceiling - floor) / wall->texture_height_pixels_;

		float half_height = height_ * 0.5f;
		float half_width = width_ * 0.5f;
		sf::Vector2f scale = camera_->GetScale();

		xyz_quad[0].y = (half_height - ceiling * scale.y / xyz_quad[0].z)  / height_;
		xyz_quad[1].y = (half_height - ceiling * scale.y / xyz_quad[1].z)  / height_;
		xyz_quad[2].y = (half_height - floor *   scale.y / xyz_quad[2].z)  / height_;
		xyz_quad[3].y = (half_height - floor *   scale.y / xyz_quad[3].z)  / height_;
											   
		xyz_quad[0].x = (half_width - xyz_quad[0].x * scale.x / xyz_quad[0].z) / width_;
		xyz_quad[1].x = (half_width - xyz_quad[1].x * scale.x / xyz_quad[1].z) / width_;
		xyz_quad[2].x = (half_width - xyz_quad[2].x * scale.x / xyz_quad[2].z) / width_;
		xyz_quad[3].x = (half_width - xyz_quad[3].x * scale.x / xyz_quad[3].z) / width_;

		int x1 = std::max(xyz_quad[0].x * width_, normalized_bounds[0].x * width_);
		x1 = Math::Clamp(0, width_, x1);
		int x2 = std::min(xyz_quad[1].x * width_, (normalized_bounds[1].x) * width_);
		x2 = Math::Clamp(0, width_, x2);

		float z_start_inv = 1 / xyz_quad[0].z;
		float z_end_inv = 1 / xyz_quad[1].z;

		// check if this is a portal
		float portal_y_boundaries[4] = { 0, 0, 0, 0 };
		if ((wall->IsPortal()) && (current_map_->nodes_[wall->node_index_] != &last_node))
		{
			Node& next_node = *current_map_->nodes_[wall->node_index_];
			float next_node_ceiling = next_node.ceiling_height_ - camera_->GetPosition().z;
			float next_node_floor = next_node.floor_height_ - camera_->GetPosition().z;
			float next_node_y[4] =
			{
				(half_height - next_node_ceiling * scale.y / xyz_quad[0].z) / height_,
				(half_height - next_node_ceiling * scale.y / xyz_quad[1].z) / height_,
				(half_height - next_node_floor * scale.y / xyz_quad[2].z) / height_,
				(half_height - next_node_floor * scale.y / xyz_quad[3].z) / height_
			};
					
			portal_y_boundaries[0] = std::max(next_node_y[0], xyz_quad[0].y);
			portal_y_boundaries[1] = std::max(next_node_y[1], xyz_quad[1].y);
			portal_y_boundaries[2] = std::min(next_node_y[2], xyz_quad[2].y);
			portal_y_boundaries[3] = std::min(next_node_y[3], xyz_quad[3].y);				
			sf::Vector2f render_bounds[4] =
			{
				{ xyz_quad[0].x, portal_y_boundaries[0] },
				{ xyz_quad[1].x, portal_y_boundaries[1] },
				{ xyz_quad[2].x, portal_y_boundaries[2] },
				{ xyz_quad[3].x, portal_y_boundaries[3] }
			};

			RenderMap(next_node, render_node, render_bounds, pixels, minimap);
		}

		for (int x = x1; x < x2; x++) 
		{
			// normalize x
			float nx = x / (float)width_;

			/*float z = (x - x1) * (tz2 - tz1) / (x2 - x1) + tz1;
			float brightness = (0.5f * -z + 100) / 100.f;
			if (brightness < 0.2f)
				brightness = 0.2f;
			if (brightness > 1)
				brightness = 1;

			sf::Color colour = sf::Color(
				(sf::Uint8)(wall->colour_.r * brightness),
				(sf::Uint8)(wall->colour_.g * brightness),
				(sf::Uint8)(wall->colour_.b * brightness));
				*/

			// linear interpolate y
			int y1 = (xyz_quad[0].y + ((xyz_quad[1].y - xyz_quad[0].y) / (xyz_quad[1].x - xyz_quad[0].x)) * (nx - xyz_quad[0].x)) * height_;
			int y2 = (xyz_quad[3].y + ((xyz_quad[2].y - xyz_quad[3].y) / (xyz_quad[1].x - xyz_quad[0].x)) * (nx - xyz_quad[0].x)) * height_;
			int y_min = (normalized_bounds[0].y + ((normalized_bounds[1].y - normalized_bounds[0].y) / (normalized_bounds[1].x - normalized_bounds[0].x)) * (nx - normalized_bounds[0].x)) * height_;
			int y_max = (normalized_bounds[3].y + ((normalized_bounds[2].y - normalized_bounds[3].y) / (normalized_bounds[1].x - normalized_bounds[0].x)) * (nx - normalized_bounds[0].x)) * height_;

			// perspective correct u
			float z = xyz_quad[0].z + ((xyz_quad[1].z - xyz_quad[0].z) / (xyz_quad[1].x - xyz_quad[0].x)) * (nx - xyz_quad[0].x);
			float u = (uv_quad[0].x * z_start_inv + ((uv_quad[1].x * z_end_inv - uv_quad[0].x * z_start_inv) / (xyz_quad[1].x - xyz_quad[0].x)) * (nx - xyz_quad[0].x)) * z;			

			u = Math::Clamp(uv_quad[0].x, uv_quad[1].x, u);
			sf::FloatRect uv = sf::FloatRect(u, 0, uv_quad[0].y, uv_quad[3].y);
			
			// fix wall tex coords
			if (y_min > y1)
			{
				uv.top = uv.top + ((uv.height - uv.top) / (float)(y2 - y1)) * (float)(y_min - y1);
				y1 = y_min;
			}

			if (y_max < y2)
			{
				uv.height = uv.top + ((uv.height - uv.top) / (y2 - y1)) * (y_max - y1);
				y2 = y_max;
			}

			// figure out where we should be drawing
			sf::IntRect ceiling_screen_space = { x, y_min, 1, y1 - y_min };
			sf::IntRect wall_screen_space = { x, y1, 1, y2 - y1 };
			sf::IntRect floor_screen_space = { x, y2, 1, y_max - y2 };

			// render floor and ceiling
			if ((y1 > y_min) || (y2 < y_max)) 
			{
				RasterizePseudoPlaneSlice(
					pixels,
					*render_node.floor_texture_,
					*render_node.ceiling_texture_,
					ceiling_screen_space,
					wall_screen_space,
					floor_screen_space);
			}			

			// interpolate portal y boundaries
			int portal_y_min = (portal_y_boundaries[0] + ((portal_y_boundaries[1] - portal_y_boundaries[0]) / (xyz_quad[1].x - xyz_quad[0].x)) * (nx - xyz_quad[0].x)) * height_;
			int portal_y_max = (portal_y_boundaries[3] + ((portal_y_boundaries[2] - portal_y_boundaries[3]) / (xyz_quad[1].x - xyz_quad[0].x)) * (nx - xyz_quad[0].x)) * height_;						
			if (portal_y_min < y_min)
				portal_y_min = y_min;

			if (portal_y_max > y_max)
				portal_y_max = y_max;

			// render wall			
			RasterizeVerticalSlice(pixels, *wall->wall_texture_, uv, wall_screen_space, { 0, portal_y_min, 0, portal_y_max - portal_y_min });
		}
	}

	minimap.draw(&lines[0], lines.size(), sf::Lines);
}

void NovaEngine::RasterizeVerticalSlice(sf::Uint8 pixels[], const class sf::Color& colour, const sf::IntRect& screen_space, const sf::IntRect& portal_screen_space)
{
	if (screen_space.top == INT_MIN || screen_space.height == INT_MIN)
		return;

	int y1 = Math::Clamp(0, height_, screen_space.top);
	int y2 = Math::Clamp(0, height_, screen_space.top + screen_space.height);
	for (int y = y1; y < y2; y++) 
	{
		if ((y >= portal_screen_space.top) && (y <= (portal_screen_space.top + portal_screen_space.height)))
			y = portal_screen_space.top + portal_screen_space.height + 1;

		if ((y < y1) || (y >= y2))
			break;

		pixels[(screen_space.left + y * width_) * 4 + 0] = colour.r;
		pixels[(screen_space.left + y * width_) * 4 + 1] = colour.g;
		pixels[(screen_space.left + y * width_) * 4 + 2] = colour.b;
		pixels[(screen_space.left + y * width_) * 4 + 3] = colour.a;
	}
}


void NovaEngine::RasterizeVerticalSlice(sf::Uint8 pixels[], const class sf::Image& texture, const sf::FloatRect& uv, const sf::IntRect& screen_space, const sf::IntRect& portal_screen_space)
{	
	if (screen_space.top == INT_MIN || screen_space.height == INT_MIN)
		return;
	
	int u = (int)(uv.left * texture.getSize().x) % texture.getSize().x;
	int y1 = Math::Clamp(0, height_, screen_space.top);
	int y2 = Math::Clamp(0, height_, screen_space.top + screen_space.height);
	for (int y = y1; y < y2; y++)
	{
		if ((y >= portal_screen_space.top) && (y <= (portal_screen_space.top + portal_screen_space.height)))
			y = portal_screen_space.top + portal_screen_space.height + 1;

		if ((y < y1) || (y >= y2))
			break;

		int v = (int)((uv.top + ((uv.height - uv.top) / ((screen_space.top + screen_space.height) - screen_space.top)) * (y - screen_space.top)) * (texture.getSize().y - 1)) % (texture.getSize().y - 1);
		sf::Color colour = texture.getPixel(u, v);
		pixels[(screen_space.left + y * width_) * 4 + 0] = colour.r;
		pixels[(screen_space.left + y * width_) * 4 + 1] = colour.g;
		pixels[(screen_space.left + y * width_) * 4 + 2] = colour.b;
		pixels[(screen_space.left + y * width_) * 4 + 3] = colour.a;
	}
}

void NovaEngine::RasterizePseudoPlaneSlice(
	sf::Uint8 pixels[],
	const class sf::Image& floor_texture,
	const class sf::Image& ceiling_texture,
	const sf::IntRect& ceiling_screen_space,
	const sf::IntRect& wall_screen_space,
	const sf::IntRect& floor_screen_space) 
{
	const Frustum& f = camera_->GetPseudoPlaneFrustum();
	int half_height = height_ / 2;
	float nx = (float)ceiling_screen_space.left / (float)width_;
	for (int y = 0; y < half_height; y++)
	{
		// in the wall's screen space
		if (((half_height - y) > wall_screen_space.top)  &&
			((half_height + y) < (wall_screen_space.top + wall_screen_space.height)))
		{
			continue;
		}

		// outside of the floor and ceiling range
		if (((half_height - y) >= (ceiling_screen_space.top + ceiling_screen_space.height)) &&
			((half_height + y) >= ((floor_screen_space.top + floor_screen_space.height))))
		{
			continue;
		}

		int offset = ((wall_screen_space.top + wall_screen_space.height) - wall_screen_space.top) / 2;
		float depth = (y + offset) / (float)(half_height + offset);
		if (depth == NAN)
			depth = FLT_EPSILON;
		if (depth == 0)
			depth = FLT_EPSILON;

		float x_start = (f.far_[0].x - f.near_[0].x) / depth + f.near_[0].x;
		float y_start = (f.far_[0].y - f.near_[0].y) / depth + f.near_[0].y;
		float x_end = (f.far_[1].x - f.near_[1].x) / depth + f.near_[1].x;
		float y_end = (f.far_[1].y - f.near_[1].y) / depth + f.near_[1].y;
		float u = fmodf(((x_end - x_start) * nx + x_start) / 20.0f, 1.0f);
		float v = fmodf(((y_end - y_start) * nx + y_start) / 20.0f, 1.0f);

		// floor
		if (((y + half_height) > floor_screen_space.top) && ((y + half_height) < (floor_screen_space.top + floor_screen_space.height)))
		{
			sf::Color colour = floor_texture.getPixel(
				Math::Clamp(0, floor_texture.getSize().x, u * floor_texture.getSize().x),
				Math::Clamp(0, floor_texture.getSize().y - 1, v * (floor_texture.getSize().y - 1)));

			pixels[(floor_screen_space.left + (y + half_height) * width_) * 4 + 0] = colour.r;
			pixels[(floor_screen_space.left + (y + half_height) * width_) * 4 + 1] = colour.g;
			pixels[(floor_screen_space.left + (y + half_height) * width_) * 4 + 2] = colour.b;
			pixels[(floor_screen_space.left + (y + half_height) * width_) * 4 + 3] = colour.a;
		}
		
		// ceiling
		if (((half_height - y) > ceiling_screen_space.top) && ((half_height - y) < (ceiling_screen_space.top + ceiling_screen_space.height)))
		{
			sf::Color colour = ceiling_texture.getPixel(
				Math::Clamp(0, ceiling_texture.getSize().x, u * ceiling_texture.getSize().x),
				Math::Clamp(0, ceiling_texture.getSize().y - 1, v * (ceiling_texture.getSize().y - 1)));

			pixels[(ceiling_screen_space.left + (half_height - y) * width_) * 4 + 0] = colour.r;
			pixels[(ceiling_screen_space.left + (half_height - y) * width_) * 4 + 1] = colour.g;
			pixels[(ceiling_screen_space.left + (half_height - y) * width_) * 4 + 2] = colour.b;
			pixels[(ceiling_screen_space.left + (half_height - y) * width_) * 4 + 3] = colour.a;
		}
	}
}

void NovaEngine::RenderNodeActors(const class Node& node, class sf::Image& pixels, const sf::FloatRect& normalized_bounds)
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