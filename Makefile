CXX = g++
CXX_FLAGS = -std=c++17 -Wall -Wextra -I./src -Idependencies/include -Iinclude /MD
GL_LIBS_LINUX = -lglfw -lGL -ldl
GL_LIBS_WINDOWS = -Ldependencies/lib -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lshell32 -lcomdlg32 -ladvapi32 -lwinmm -lws2_32

GL_LIBS = $(GL_LIBS_WINDOWS)

CUDA = nvcc
CUDA_FLAGS = -arch=sm_61 -I./src -Iinclude -Idependencies/include --expt-relaxed-constexpr -Xcompiler="/W3 /EHsc /MD"
CUDA_LIBS = -lcuda -lcudart

# Build output directory for object files
OBJDIR = obj
BINDIR = bin

# sources
COMMON_SRC = src/opengl/shader.cpp \
		src/opengl/VAO.cpp \
		src/opengl/VBO.cpp \
		src/opengl/EBO.cpp \
		src/ray-tracer/camera.cpp\
		src/ray-tracer/ray.cpp\
		src/ray-tracer/triangle.cpp\

GLAD_SRC = src/glad.c
MAIN_SRC = src/main.cpp

TEST_GLFW_SRC = src/test/test_glfw.cpp
TEST_SRC = src/test/test.cpp
LOADER_SRC = src/test/obj_loader.cpp
EBO_SRC = src/test/ebo.cpp
TEXTURE_SRC = src/test/texture.cpp
LIGHT_SRC = src/test/light.cpp

TARGETS = main obj_loader test_glfw ebo test texture light

COMMON_OBJ = $(COMMON_SRC:src/%.cpp=$(OBJDIR)/%.o)
GLAD_OBJ = $(GLAD_SRC:src/%.cpp=$(OBJDIR)/%.o)
MAIN_OBJ = $(MAIN_SRC:src/%.cpp=$(OBJDIR)/%.o)

TEST_GLFW_OBJ = $(TEST_GLFW_SRC:src/%.cpp=$(OBJDIR)/%.o)
TEST_OBJ = $(TEST_SRC:src/%.cpp=$(OBJDIR)/%.o)
LOADER_OBJ = $(LOADER_SRC:src/%.cpp=$(OBJDIR)/%.o)
EBO_OBJ = $(EBO_SRC:src/%.cpp=$(OBJDIR)/%.o)
TEXTURE_OBJ = $(TEXTURE_SRC:src/%.cpp=$(OBJDIR)/%.o)
LIGHT_OBJ = $(LIGHT_SRC:src/%.cpp=$(OBJDIR)/%.o)


# Ensure the bin directory exists
$(BINDIR):
	@mkdir -p $(BINDIR)

all: $(BINDIR) $(TARGETS)

# to build the target, we need all object files, but we dont have them yet!
# compile the object files using the %.c:%.cpp rule
# now that they are built, use them (%^) to create the final target ($@)

$(BINDIR)/main: $(MAIN_OBJ) ${COMMON_OBJ}
	$(CXX) $(CXX_FLAGS) -o $@ $^

$(BINDIR)/obj_loader: $(LOADER_OBJ) ${OBJDIR}/math/vector3f.o
	$(CXX) $(CXX_FLAGS) -o $@ $^

$(BINDIR)/test_glfw: $(TEST_GLFW_OBJ) $(GLAD_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/ebo: $(EBO_OBJ) $(GLAD_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/test: $(TEST_OBJ) $(GLAD_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/texture: $(TEXTURE_OBJ) $(GLAD_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)

$(BINDIR)/light: $(LIGHT_OBJ) $(GLAD_OBJ) $(COMMON_OBJ)
	$(CXX) $(CXX_FLAGS) -o $@ $^ $(GL_LIBS)





$(BINDIR)/cuda_opengl_interop.run: src/test/cuda_opengl_interop.cu
	$(CUDA) -o $@ $< $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)
$(BINDIR)/hello.run: src/test/hello.cu
	$(CUDA) -o $@ $< $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)


# @mkdir -p $(dir $@)
$(OBJDIR)/%.o : src/%.cpp
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CXX) $(CXX_FLAGS) -c $< -o $@

# just for glad.c since it's a c file
# @mkdir -p $(dir $@)
$(OBJDIR)/%.o : %.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CXX) $(CXX_FLAGS) -c $< -o $@


run-%: $(BINDIR)/%
	@echo "Running $<..."
	@./$<

run-cuda-%: $(BINDIR)/%.run
	@echo "Running CUDA $<..."
	@./$<

# rm -f $(BINDIR)/* $(OBJDIR)/*.o
clean:
	@if exist "$(BINDIR)" del /Q "$(BINDIR)\*"
	@for /R "$(OBJDIR)" %%f in (*.o) do del "%%f"