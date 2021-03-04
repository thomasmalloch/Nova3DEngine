#include "Nova3DEngine.h"

using namespace nova;

Camera::Camera(float screen_width, float screen_height, float frustrum_near, float frustrum_far)
{
	position_ = { 0, 0, 0 };
	angle_ = 0;
	cos_angle_ = cosf(angle_);
	sin_angle_ = sinf(angle_);
	screen_width_ = screen_width;
	screen_height_ = screen_height;
	near_ = frustrum_near;
	far_ = frustrum_far;
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
	wall_clipping_planes_->near_[0].x = 1000.f * cosf(-fov_half_);
	wall_clipping_planes_->near_[0].y = 1000.f * sinf(-fov_half_);
	wall_clipping_planes_->far_[0].x = 0.1f * cosf(-fov_half_);
	wall_clipping_planes_->far_[0].y = 0.1f * sinf(-fov_half_);

	wall_clipping_planes_->near_[1].x = 1000.f * cosf(fov_half_);
	wall_clipping_planes_->near_[1].y = 1000.f * sinf(fov_half_);
	wall_clipping_planes_->far_[1].x = 0.1f * cosf(fov_half_);
	wall_clipping_planes_->far_[1].y = 0.1f * sinf(fov_half_);
}

void Camera::UpdatePseudoPlaneFrustrum() 
{
	pseudo_plane_frustrum_->far_[0].x = ((position_.x + 1000.f) + cosf(angle_ - fov_half_) * far_);
	pseudo_plane_frustrum_->far_[0].y = (position_.y + 1000.f + sinf(angle_ - fov_half_) * far_);
	pseudo_plane_frustrum_->near_[0].x = (position_.x + 1000.f + cosf(angle_ - fov_half_) * near_);
	pseudo_plane_frustrum_->near_[0].y = (position_.y + 1000.f + sinf(angle_ - fov_half_) * near_);

	pseudo_plane_frustrum_->far_[1].x = (position_.x + 1000.f + cosf(angle_ + fov_half_) * far_);
	pseudo_plane_frustrum_->far_[1].y = (position_.y + 1000.f + sinf(angle_ + fov_half_) * far_);
	pseudo_plane_frustrum_->near_[1].x = (position_.x + 1000.f + cosf(angle_ + fov_half_) * near_);
	pseudo_plane_frustrum_->near_[1].y = (position_.y + 1000.f + sinf(angle_ + fov_half_) * near_);
}

void Camera::SetPosition(float x, float y, float z) 
{
	SetPosition({x, y, z});
}

void Camera::SetPosition(sf::Vector3f position) 
{
	position_ = position;
	UpdatePseudoPlaneFrustrum();
}

void Camera::SetAngle(float angle)
{
	angle_ = angle;
	cos_angle_ = cosf(angle);
	sin_angle_ = sinf(angle);
	UpdatePseudoPlaneFrustrum();
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
	UpdatePseudoPlaneFrustrum();
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

float Camera::GetNear()
{
	return near_;
}

float Camera::GetFar()
{
	return far_;
}

void Camera::SetNear(float pseudo_plane_near)
{
	near_ = pseudo_plane_near;
	UpdatePseudoPlaneFrustrum();
}

void Camera::SetFar(float pseudo_plane_far)
{
	far_ = pseudo_plane_far;
	UpdatePseudoPlaneFrustrum();
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

const class Frustum& Camera::GetPseudoPlaneFrustum()
{
	return *pseudo_plane_frustrum_;
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