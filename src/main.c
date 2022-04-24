#include "main.h"
#include "models/mario.h"

GLuint gl_shader_program;

Camera camera = {
    .scale = {
        1.0f, 1.0f, 1.0f
    }
};

SDL_KeyCode keys_pressed[100];
int keys_pressed_size = 0;

bool is_key_pressed(SDL_KeyCode key) {
    for (int i = 0; i < keys_pressed_size; i++) {
        if (keys_pressed[i] == key) return true;
    }
    return false;
}

Entity ent1 = {
    .position = { 0.0f, 0.0f, 0.0f },
    .rotation = { 0.0f, 0.0f, 0.0f },
    .scale = { 1.0f, 1.0f, 1.0f },
};

int main(void)
{
    return start_sdl_and_main_loop(main_loop, dispose);
}

static bool is_first_loop = true;

bool main_loop(float delta)
{
    if (is_first_loop)
    {
        is_first_loop = false;
        gl_shader_program = init_shaders("assets/vert.glsl", "assets/frag.glsl");
        if (gl_shader_program == 0)
        {
            return true;
        }

        ent1.mesh = mario;
        init_entity(&ent1);

        vec3_set_values(&ent1.scale, 0.2f, 0.2f, 0.2f);
        ent1.position.z = -5.f;

        // vec3_set_values(&camera.scale, 0.1f, 0.1f, 0.1f);
        camera.position.z = 1.f;
    }

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            return true;
        }
        else if (e.type == SDL_KEYDOWN && e.key.repeat == 0)
        {
            keys_pressed[keys_pressed_size] = e.key.keysym.sym;
            keys_pressed_size++;
        }
        else if (e.type == SDL_KEYUP && e.key.repeat == 0)
        {
            SDL_KeyCode key = e.key.keysym.sym;
            for (int i = 0; i < keys_pressed_size; i++) {
                if (keys_pressed[i] == key) {
                    // displace the rest of the array 1 spot to the left
                    if (i < keys_pressed_size - 1) {
                        for (int j = i; j < keys_pressed_size - 1; j++) {
                            keys_pressed[j] = keys_pressed[j + 1];
                        }
                    }
                    keys_pressed_size--;
                    break;
                }
            }
        }
    }

    float cam_speed = 0.1f;
    float cam_rot_speed = 2.0f;

    if (is_key_pressed(SDLK_ESCAPE))
        return true;

    if (is_key_pressed(SDLK_w))
        camera.position.z -= cam_speed;
    if (is_key_pressed(SDLK_s))
        camera.position.z += cam_speed;
    if (is_key_pressed(SDLK_a))
        camera.position.x -= cam_speed;
    if (is_key_pressed(SDLK_d))
        camera.position.x += cam_speed;
    if (is_key_pressed(SDLK_q))
        camera.position.y += cam_speed;
    if (is_key_pressed(SDLK_e))
        camera.position.y -= cam_speed;

    if (is_key_pressed(SDLK_k))
        camera.rotation.x += cam_rot_speed;
    if (is_key_pressed(SDLK_i))
        camera.rotation.x -= cam_rot_speed;
    if (is_key_pressed(SDLK_j))
        camera.rotation.y -= cam_rot_speed;
    if (is_key_pressed(SDLK_l))
        camera.rotation.y += cam_rot_speed;
    if (is_key_pressed(SDLK_u))
        camera.rotation.z += cam_rot_speed;
    if (is_key_pressed(SDLK_o))
        camera.rotation.z -= cam_rot_speed;

    glViewport(0, 0, screen_width, screen_height);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.2f, 0.5f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(gl_shader_program);

    Mat4 projection;
    set_projection_matrix(&projection, 90.0f, 0.1f, 100.0f, screen_width, screen_height);

    GLuint id = glGetUniformLocation(gl_shader_program, "proj_transform");
    glUniformMatrix4fv(id, 1, GL_FALSE, projection);

    camera_update_transform(&camera);
    id = glGetUniformLocation(gl_shader_program, "view_transform");
    glUniformMatrix4fv(id, 1, GL_FALSE, camera.world_transform);

    // vec3_add_values(&ent1.scale, -0.01f, -0.01f, -0.01f);

    // ent1.position.z -= 0.1f;
    // ent1.position.x += 0.002f;
    // ent1.rotation.z += 2.0f;
    // ent1.rotation.z += 1.f;
    entity_update_transform(&ent1);

    draw_entity(&ent1);

    glUseProgram(0);

    SDL_GL_SwapWindow(sdl_window);

    return false;
}

void camera_update_transform(Camera* camera) {
    mat4_set_identity(&camera->world_transform);

    vec3_scl(&camera->position, -1.f, -1.f, -1.f);
    mat4_translate_by_vec3(&camera->world_transform, camera->position);
    vec3_scl(&camera->position, -1.f, -1.f, -1.f);

    mat4_rotate_around_axis(&camera->world_transform, X_AXIS, camera->rotation.x);
    mat4_rotate_around_axis(&camera->world_transform, Y_AXIS, camera->rotation.y);
    mat4_rotate_around_axis(&camera->world_transform, Z_AXIS, camera->rotation.z);
    mat4_scale_by_vec3(&camera->world_transform, camera->scale);
}

void entity_update_transform(Entity* ent) {
    mat4_set_identity(&ent->world_transform);
    mat4_translate_by_vec3(&ent->world_transform, ent->position);
    mat4_rotate_around_axis(&ent->world_transform, X_AXIS, ent->rotation.x);
    mat4_rotate_around_axis(&ent->world_transform, Y_AXIS, ent->rotation.y);
    mat4_rotate_around_axis(&ent->world_transform, Z_AXIS, ent->rotation.z);
    mat4_scale_by_vec3(&ent->world_transform, ent->scale);
}

void mesh_apply_transform(Mesh* mesh, Mat4* transform) {
    for (int i = 0; i < ARRAY_LENGTH_STACK(mesh->vertices); i++) {
        for (int j = 0; j < ARRAY_LENGTH_STACK(mesh->vertices[i]); j++) {
            Vec3 tmp;
            tmp.x = mesh->vertices[i][0];
            tmp.y = mesh->vertices[i][1];
            tmp.z = mesh->vertices[i][2];
            vec3_transform_by_mat4(&tmp, transform);
            mesh->vertices[i][0] = tmp.x;
            mesh->vertices[i][1] = tmp.y;
            mesh->vertices[i][2] = tmp.z;
        }
    }
}

void bind_mesh_to_opengl(Mesh* mesh)
{
    glGenVertexArrays(1, &mesh->vao);
    glBindVertexArray(mesh->vao);

    // Populate vertex buffer
    glGenBuffers(1, &mesh->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(mesh->vertices[0]) * mesh->vertices_size, mesh->vertices, GL_STATIC_DRAW);

    // Populate element buffer
    glGenBuffers(1, &mesh->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(mesh->indices[0]) * mesh->indices_size, mesh->indices, GL_STATIC_DRAW);

    // Bind vertex position attribute
    GLint pos_attr_loc = glGetAttribLocation(gl_shader_program, "in_position");
    glVertexAttribPointer(pos_attr_loc, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(pos_attr_loc);

    // Bind vertex texture coordinate attribute
    // GLint tex_attr_loc = glGetAttribLocation(gl_shader_program, "in_Texcoord");
    // glVertexAttribPointer(tex_attr_loc, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (void*)(2*sizeof(GLfloat)));
    // glEnableVertexAttribArray(tex_attr_loc);

    glBindVertexArray(0);
}

void init_entity(Entity* ent)
{
    bind_mesh_to_opengl(&ent->mesh);
}

void draw_entity(Entity* ent) {
    GLuint id = glGetUniformLocation(gl_shader_program, "local_transform");
    glUniformMatrix4fv(id, 1, GL_FALSE, ent->world_transform);

    draw_mesh(&ent->mesh);
}

void check_gl_errors(char* context) {
    GLenum error = glGetError();
    if (GL_NO_ERROR != error) {
        printf("GL Error %x encountered in %s.\n", error, context);
        exit_app();
    }
}

void draw_mesh(Mesh* mesh)
{
    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);

    glDrawElements(GL_TRIANGLES, mesh->indices_size, GL_UNSIGNED_INT, NULL);

    glBindVertexArray(0);
}

int init_shaders(char* vert_shader_filename, char* frag_shader_filename)
{
    size_t size;
    const char* vert_shader = read_file(vert_shader_filename, &size);
    const char* frag_shader = read_file(frag_shader_filename, &size);

    GLuint new_program;

    GLint status;
    char err_buf[512];

    // vertex
    GLuint vertex_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_id, 1, &vert_shader, NULL);
    glCompileShader(vertex_id);
    glGetShaderiv(vertex_id, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        glGetShaderInfoLog(vertex_id, sizeof(err_buf), NULL, err_buf);
        err_buf[sizeof(err_buf) - 1] = '\0';
        fprintf(stderr, "Vertex shader compilation failed: %s\n", err_buf);
        return 0;
    }

    // fragment
    GLuint fragment_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_id, 1, &frag_shader, NULL);
    glCompileShader(fragment_id);
    glGetShaderiv(fragment_id, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        glGetShaderInfoLog(fragment_id, sizeof(err_buf), NULL, err_buf);
        err_buf[sizeof(err_buf) - 1] = '\0';
        fprintf(stderr, "Fragment shader compilation failed: %s\n", err_buf);
        return 0;
    }

    new_program = glCreateProgram();
    glAttachShader(new_program, vertex_id);
    glAttachShader(new_program, fragment_id);

    glBindFragDataLocation(new_program, 0, "out_color");

    glLinkProgram(new_program);
    glUseProgram(new_program);

    glGetProgramiv(new_program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        glGetProgramInfoLog(new_program, sizeof(err_buf), NULL, err_buf);
        err_buf[sizeof(err_buf) - 1] = '\0';
        fprintf(stderr, "Program (shader) linking failed: %s\n", err_buf);
        return 0;
    }

    glDetachShader(new_program, fragment_id);
    glDetachShader(new_program, vertex_id);

    glDeleteShader(fragment_id);
    glDeleteShader(vertex_id);

    glUseProgram(0);

    return new_program;
}


void dispose() {

}
