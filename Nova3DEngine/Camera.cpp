#include "Nova3DEngine.h"

using namespace nova;

Camera::Camera(float screen_width, float screen_height)
{
	position_ = { 0, 0, 0 };
	angle_ = 0;
	cos_angle_ = cosf(angle_);
	sin_angle_ = sinf(angle_);
	screen_width_ = screen_width;
	screen_height_ = screen_height;
	wall_clipping_planes_ = new Frustum();
	pseudo_plane_frustrum_ = new Frustum();
	SetFOVRadians(Math::half_pi_); // 90 degrees
}

Camera::~Camera() 
{
	delete wall_clipping_planes_;
	delete pseudo_plane_frustrum_;
}

void Camera::UpdateWallClippingPlanes() 
{
	float x1 = 1.f * cosf(-fov_half_);
	float z1 = 1.f * sinf(-fov_half_);
	float x2 = 0.1f * cosf(-fov_half_);
	float z2 = 0.1f * sinf(-fov_half_);	
	Plane left = Plane({ x1, 0, z1 }, {});

	x1 = 0.1f * cosf(fov_half_);
	z1 = 0.1f * sinf(fov_half_);
	x2 = 1.f * cosf(fov_half_);
	z2 = 1.f * sinf(fov_half_);
	Plane right = Plane({ x1, 0, z1 }, {});


	Plane near = Plane({ 0, 0, 0.1f }, {0, 0, 1});

	// should we add a far?

	wall_clipping_planes_->near_[0].x = 1000.f * cosf(-fov_half_);
	wall_clipping_planes_->near_[0].y = 1000.f * sinf(-fov_half_);
	wall_clipping_planes_->far_[0].x = 0.1f * cosf(-fov_half_);
	wall_clipping_planes_->far_[0].y = 0.1f * sinf(-fov_half_);

	wall_clipping_planes_->near_[1].x = 1000.f * cosf(fov_half_);
	wall_clipping_planes_->near_[1].y = 1000.f * sinf(fov_half_);
	wall_clipping_planes_->far_[1].x = 0.1f * cosf(fov_half_);
	wall_clipping_planes_->far_[1].y = 0.1f * sinf(fov_half_);
}

void Camera::SetPosition(float x, float y, float z) 
{
	SetPosition({x, y, z});
}

void Camera::SetPosition(sf::Vector3f position) 
{
	position_ = position;
}

void Camera::SetAngle(float angle)
{
	angle_ = angle;
	cos_angle_ = cosf(angle);
	sin_angle_ = sinf(angle);
}

void Camera::SetFOVDegrees(float fov_degrees)
{
	SetFOVRadians(fov_degrees * Math::pi_ / 180.f);
}

void Camera::SetFOVRadians(float fov_rad)
{
	fov_ = fov_rad;
	fov_half_ = 0.5f * fov_rad;
	float scale = 1 / tanf(fov_half_);
	float aspect = screen_width_ / screen_height_;
	x_scale_ = 0.5 * screen_width_ * scale / std::min(1.f, aspect);
	y_scale_ = 0.5 * screen_height_ * scale / std::min(1.f, aspect);
	UpdateWallClippingPlanes();
}

float Camera::GetFOV()
{
	return fov_;
}

float Camera::GetFOVHalf()
{
	return fov_half_;
}

float Camera::GetAngle()
{
	return angle_;
}

float Camera::GetCosAngle()
{
	return cos_angle_;
}

float Camera::GetSinAngle()
{
	return sin_angle_;
}

const sf::Vector2f& Camera::GetScale()
{
	return { x_scale_, y_scale_ };
}

void Camera::SetCurrentNode(class Node* node)
{
	current_node_ = node;
}

class Node* Camera::GetCurrentNode()
{
	return current_node_;
}

const class Frustum& Camera::GetWallClippingPlanes()
{
	return *wall_clipping_planes_;
}

const sf::Vector3f& Camera::GetPosition() 
{
	return position_;
}

sf::Vector2f Camera::Project(sf::Vector3f p)
{
	return
	{
		0.5f * screen_width_ + x_scale_ * p.x / p.z,
		0.5f * screen_height_ + y_scale_ * p.y / p.z
	};
}

sf::Vector3f Camera::InverseProject(sf::Vector2f p, float z)
{
	return
	{
		(p.x - 0.5f * screen_width_) * z / x_scale_,
		(p.y - 0.5f * screen_height_) * z / y_scale_,
		z
	};
}