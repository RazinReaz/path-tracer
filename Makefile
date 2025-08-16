# CXX = g++
# CXX_FLAGS = -std=c++17 -Wall -Wextra -I./src -Idependencies/include -Iinclude
CXX = cl
CXX_FLAGS = /std:c++17 /EHsc /W3 /I./src /Idependencies/include /Iinclude /MD 
GL_LIBS_LINUX = -lglfw -lGL -ldl
GL_LIBS_WINDOWS = -Ldependencies/lib -lglfw3 -lopengl32 -lgdi32 -luser32 -lkernel32 -lshell32 -lcomdlg32 -ladvapi32 -lwinmm -lws2_32

GL_LIBS = $(GL_LIBS_WINDOWS)

CUDA = nvcc
CUDA_FLAGS = -arch=sm_61 -I./src -Iinclude -Idependencies/include --expt-relaxed-constexpr -Xcompiler="/W3 /EHsc /MD"
CUDA_LIBS = -lcuda -lcudart

# Build output directory for object files
OBJDIR = obj
# CUDA_OBJDIR = $(OBJDIR)/cuda
BINDIR = bin

# sources
CPP_SRC = src/opengl/shader.cpp \
		src/opengl/VAO.cpp \
		src/opengl/VBO.cpp \
		src/opengl/EBO.cpp \


GLAD_SRC = src/glad.c

CPP_OBJ = $(CPP_SRC:src/%.cpp=$(OBJDIR)/%.obj)
# CUDA_OBJ = $(CUDA_SRC:src/%.cu=$(CUDA_OBJDIR)/%.obj)

GLAD_OBJ = $(GLAD_SRC:src/%.c=$(OBJDIR)/%.obj)


# Ensure the bin directory exists
$(BINDIR):
	@mkdir -p $(BINDIR)

all: $(BINDIR) $(TARGETS)

# to build the target, we need all object files, but we dont have them yet!
# compile the object files using the %.c:%.cpp rule
# now that they are built, use them (%^) to create the final target ($@)


# @mkdir -p $(dir $@)
$(OBJDIR)/%.obj : src/%.cpp
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CXX) $(CXX_FLAGS) /c $< /Fo$@

# # Rule for CUDA files
# $(CUDA_OBJDIR)/%.obj : src/%.cu
# 	@if not exist "$(dir $@)" mkdir "$(dir $@)"
# 	$(CUDA) $(CUDA_FLAGS) -c $< -o $@

# just for glad.c since it's a c file
# @mkdir -p $(dir $@)
$(OBJDIR)/%.obj : %.c
	@if not exist "$(dir $@)" mkdir "$(dir $@)"
	$(CXX) $(CXX_FLAGS) /c $< /Fo$@


$(BINDIR)/0.hello.run: src/test-cuda/0.hello.cu
	$(CUDA) -o $@ $< $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)
$(BINDIR)/1.cuda_opengl_interop.run: src/test-cuda/1.cuda_opengl_interop.cu $(CPP_OBJ)
	$(CUDA) -o $@ $^ $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)
$(BINDIR)/2.movement.run: src/test-cuda/2.movement.cu $(CPP_OBJ)
	$(CUDA) -o $@ $^ $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)
$(BINDIR)/3.scene_load.run: src/test-cuda/3.scene_load.cu $(CPP_OBJ)
	$(CUDA) -o $@ $^ $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)
$(BINDIR)/4.materials.run: src/test-cuda/4.materials.cu $(CPP_OBJ)
	$(CUDA) -o $@ $^ $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)
$(BINDIR)/5.bvh.run: src/test-cuda/5.bvh.cu $(CPP_OBJ)
	$(CUDA) -o $@ $^ $(GLAD_SRC) $(CUDA_FLAGS) $(CUDA_LIBS) $(GL_LIBS)

$(BINDIR)/test/random-unit-vector.run: src/test/random-unit-vector.cu
	$(CUDA) -o $@ $^ $(CUDA_FLAGS) $(CUDA_LIBS)

# BVH test targets
$(BINDIR)/test_bvh.run: src/test/test_bvh.cu
	$(CUDA) -o $@ $^ $(CUDA_FLAGS) $(CUDA_LIBS)

$(BINDIR)/benchmark_bvh.run: src/benchmark/benchmark_bvh.cu
	$(CUDA) -o $@ $^ $(CUDA_FLAGS) $(CUDA_LIBS)


run-%: $(BINDIR)/%.run
	@echo "Running CUDA $<..."
	@./$<

run-test-%: $(BINDIR)/test/%.run
	@echo "Running CUDA $<..."
	@./$<

# BVH test run targets
run-bvh-test: $(BINDIR)/test_bvh.run
	@echo "Running BVH test..."
	@./$<

run-bvh-simple: $(BINDIR)/test/test_bvh_simple.run
	@echo "Running simple BVH test..."
	@./$<

run-bvh-benchmark: $(BINDIR)/benchmark_bvh.run
	@echo "Running BVH benchmark..."
	@./$<

# rm -f $(BINDIR)/* $(OBJDIR)/*.o
clean:
	@if exist "$(BINDIR)" (
		@rmdir /S /Q "$(BINDIR)"
		@mkdir "$(BINDIR)"
	)
	# @for /R "$(OBJDIR)" %%f in (*.obj) do del "%%f" 2>nul