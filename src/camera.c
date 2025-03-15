#include "camera.h"
#include "cglm/vec3.h"
#include "constants.h"

void camera_set_view(Camera *camera) {
    glm_lookat(camera->position, camera->target, camera->up, camera->view);
    glm_mat4_mul(camera->perspective, camera->view, camera->mvp);
    glm_mat4_mul(camera->mvp, camera->model, camera->mvp);
}

void camera_init(Camera *camera) {
    glm_mat4_identity(camera->mvp);
    // setup model view projection matrix
    {
        // "Flatten" the world using the given scale
        mat4 ortho = {0};
        glm_ortho(0, 100, 0, 100, -1, 1, ortho);

        // "Translate" the camera by the translation vector (in reverse)
        glm_vec3_copy((vec3){30, 50, 100}, camera->position);
        glm_vec3_copy((vec3){50, 50, 0}, camera->target);
        glm_vec3_copy((vec3){0, 1, 0}, camera->up);
        glm_mat4_identity(camera->view);
        glm_lookat(camera->position, camera->target, camera->up, camera->view);

        glm_mat4_identity(camera->model);
        glm_translate(camera->model, (vec3){0, 0, 0});

        glm_mat4_identity(camera->perspective);
        glm_perspective(glm_rad(90), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0, 1, camera->perspective);

        glm_mat4_mul(camera->perspective, camera->view, camera->mvp);
        glm_mat4_mul(camera->mvp, camera->model, camera->mvp);
    }
}

void camera_rotate_around_point(Camera *camera, vec3 point, CameraDirection rotation, float amount) {
    vec3 movement = {0};
    glm_vec3_sub(point, camera->position, movement);
    vec3 mv = {-movement[2], movement[0], 0};
    glm_normalize(mv);
    glm_vec3_scale(mv, amount, mv);
    switch (rotation) {
    case CAMERA_DIRECTION_LEFT: {
        camera->position[0] -= mv[0];
        camera->position[2] -= mv[1];
    } break;
    case CAMERA_DIRECTION_RIGHT: {
        camera->position[0] += mv[0];
        camera->position[2] += mv[1];
    } break;
    }
    camera_set_view(camera);
}

void camera_strafe(Camera *camera, CameraDirection rotation, float amount) {
    vec3 movement = {0};
    glm_vec3_sub(camera->target, camera->position, movement);
    vec3 mv = {-movement[2], movement[0], 0};
    glm_normalize(mv);
    glm_vec3_scale(mv, amount, mv);
    switch (rotation) {
    case CAMERA_DIRECTION_LEFT: {
        camera->position[0] -= mv[0];
        camera->position[2] -= mv[1];
        camera->target[0] -= mv[0];
        camera->target[2] -= mv[1];
    } break;
    case CAMERA_DIRECTION_RIGHT: {
        camera->position[0] += mv[0];
        camera->position[2] += mv[1];
        camera->target[0] += mv[0];
        camera->target[2] += mv[1];
    } break;
    }
    camera_set_view(camera);
}

void camera_zoom(Camera *camera, CameraZoom zoom, float amount) {
    vec3 dest = {0};
    glm_vec3_sub(camera->position, camera->target, dest);
    glm_normalize(dest);
    glm_vec3_scale(dest, amount, dest);
    switch (zoom) {
    case CAMERA_ZOOM_IN: {
        glm_vec3_add(camera->position, dest, camera->position);
    } break;
    case CAMERA_ZOOM_OUT: {
        glm_vec3_sub(camera->position, dest, camera->position);
    } break;
    }
    camera_set_view(camera);
}
