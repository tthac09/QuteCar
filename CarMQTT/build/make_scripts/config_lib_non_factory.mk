ifeq ($(CONFIG_TARGET_CHIP_HI3861), y)
	ifeq ($(CONFIG_DIAG_SUPPORT), y)
		ifeq ($(CONFIG_MESH_SUPPORT), y)
			LIBPATH += -Lbuild/libs/hi3861/debug/mesh
		else
			LIBPATH += -Lbuild/libs/hi3861/debug/no_mesh
		endif
	endif
else
	ifeq ($(CONFIG_DIAG_SUPPORT), y)
		ifeq ($(CONFIG_MESH_SUPPORT), y)
			LIBPATH += -Lbuild/libs/hi3861l/debug/mesh
		else
			LIBPATH += -Lbuild/libs/hi3861l/debug/no_mesh
		endif
	endif
endif

ifeq ($(CONFIG_TARGET_CHIP_HI3861), y)
	ifneq ($(CONFIG_DIAG_SUPPORT), y)
		ifeq ($(CONFIG_MESH_SUPPORT), y)
			LIBPATH += -Lbuild/libs/hi3861/release/mesh
		else
			LIBPATH += -Lbuild/libs/hi3861/release/no_mesh
		endif
	endif
else
	ifneq ($(CONFIG_DIAG_SUPPORT), y)
		ifeq ($(CONFIG_MESH_SUPPORT), y)
			LIBPATH += -Lbuild/libs/hi3861l/release/mesh
		else
			LIBPATH += -Lbuild/libs/hi3861l/release/no_mesh
		endif
	endif
endif
