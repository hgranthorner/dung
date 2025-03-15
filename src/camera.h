#pragma once

#include <cglm/cglm.h>

typedef struct {
    vec3 position;
    vec3 target;
    vec3 up;
    mat4 view;

    mat4 perspective;
    mat4 model;
    mat4 mvp;
} Camera;

typedef enum { CAMERA_DIRECTION_LEFT, CAMERA_DIRECTION_RIGHT } CameraDirection;

void camera_set_view(Camera *camera);
void camera_init(Camera *camera);
void camera_rotate_around_point(Camera *camera, vec3 point, CameraDirection rotation, float amount);
void camera_strafe(Camera *camera, CameraDirection rotation, float amount);
