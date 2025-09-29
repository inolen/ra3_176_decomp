.PHONY: default clean makedirs debug

# ------------------------------------------------------------------------------
# targets
# ------------------------------------------------------------------------------

default: debug

clean:
	rm -rf $(OBJDIR)

makedirs:
	@mkdir -p $(dir $(QAGAME_OBJ))

debug: makedirs
	@$(MAKE) CONFIGURATION=debug $(QAGAME)

# ------------------------------------------------------------------------------
# common rules
# ------------------------------------------------------------------------------

SRCDIR = code
OBJDIR = build
BINEXT =
LIBEXT = .so

CFLAGS = -MMD -fPIC
LDFLAGS = -shared -lpthread -Wl,-Map,$(OBJDIR)/qagamei386.map

ifeq ($(CONFIGURATION),debug)
  CFLAGS += -O0
endif

ifneq ($(ENABLE_LINKER_SCRIPT),0)
  LDFLAGS += -T qagamei386.lds
endif

ifeq ($(V),1)
  ECHO = @:
  Q =
else
  ECHO = @echo
  Q = @
endif

# use cd / notdir so __FILE__ doesn't include the directory path
# the first sed strips the base path from the target, and the
# second adds the relative path back to any inputs
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(ECHO) "CC $<"
	$(Q)cd $(dir $<) && \
	$(CC) $(CFLAGS) -o $(CURDIR)/$@ -c $(notdir $<) && \
	sed -i 's#$(CURDIR)/##g' $(CURDIR)/$(OBJDIR)/$*.d && \
	sed -i 's#\([^ ]\+\.[c|h]\)#$(dir $<)\1#g' $(CURDIR)/$(OBJDIR)/$*.d

# ------------------------------------------------------------------------------
# qagame rules
# ------------------------------------------------------------------------------

SQLITE_SRC = \
  $(SRCDIR)/sqlite/sql_auth.c \
  $(SRCDIR)/sqlite/sql_btree.c \
  $(SRCDIR)/sqlite/sql_build.c \
  $(SRCDIR)/sqlite/sql_delete.c \
  $(SRCDIR)/sqlite/sql_expr.c \
  $(SRCDIR)/sqlite/sql_func.c \
  $(SRCDIR)/sqlite/sql_hash.c \
  $(SRCDIR)/sqlite/sql_insert.c \
  $(SRCDIR)/sqlite/sql_main.c \
  $(SRCDIR)/sqlite/sql_opcodes.c \
  $(SRCDIR)/sqlite/sql_os.c \
  $(SRCDIR)/sqlite/sql_pager.c \
  $(SRCDIR)/sqlite/sql_parse.c \
  $(SRCDIR)/sqlite/sql_printf.c \
  $(SRCDIR)/sqlite/sql_random.c \
  $(SRCDIR)/sqlite/sql_select.c \
  $(SRCDIR)/sqlite/sql_shell.c \
  $(SRCDIR)/sqlite/sql_table.c \
  $(SRCDIR)/sqlite/sql_tclsqlite.c \
  $(SRCDIR)/sqlite/sql_tokenize.c \
  $(SRCDIR)/sqlite/sql_trigger.c \
  $(SRCDIR)/sqlite/sql_update.c \
  $(SRCDIR)/sqlite/sql_util.c \
  $(SRCDIR)/sqlite/sql_vdbe.c \
  $(SRCDIR)/sqlite/sql_where.c
SQLITE_OBJ = $(SQLITE_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(SQLITE_OBJ): CFLAGS+=-DNO_TCL

HTTPD_SRC = \
  $(SRCDIR)/httpd/api.c \
  $(SRCDIR)/httpd/ip_acl.c \
  $(SRCDIR)/httpd/protocol.c
HTTPD_OBJ = $(HTTPD_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

GAME_SRC = \
  $(SRCDIR)/game/ai_chat.c \
  $(SRCDIR)/game/ai_cmd.c \
  $(SRCDIR)/game/ai_dmnet.c \
  $(SRCDIR)/game/ai_dmq3.c \
  $(SRCDIR)/game/ai_main.c \
  $(SRCDIR)/game/ai_team.c \
  $(SRCDIR)/game/arena.c \
  $(SRCDIR)/game/arenacfg.c \
  $(SRCDIR)/game/arena_common.c \
  $(SRCDIR)/game/bg_lib.c \
  $(SRCDIR)/game/bg_misc.c \
  $(SRCDIR)/game/bg_pmove.c \
  $(SRCDIR)/game/bg_slidemove.c \
  $(SRCDIR)/game/g_active.c \
  $(SRCDIR)/game/g_arenas.c \
  $(SRCDIR)/game/g_bot.c \
  $(SRCDIR)/game/g_client.c \
  $(SRCDIR)/game/g_cmds.c \
  $(SRCDIR)/game/g_combat.c \
  $(SRCDIR)/game/g_items.c \
  $(SRCDIR)/game/g_main.c \
  $(SRCDIR)/game/g_mem.c \
  $(SRCDIR)/game/g_misc.c \
  $(SRCDIR)/game/g_missile.c \
  $(SRCDIR)/game/g_mover.c \
  $(SRCDIR)/game/g_session.c \
  $(SRCDIR)/game/g_spawn.c \
  $(SRCDIR)/game/g_svcmds.c \
  $(SRCDIR)/game/g_syscalls.c \
  $(SRCDIR)/game/g_target.c \
  $(SRCDIR)/game/g_team.c \
  $(SRCDIR)/game/g_trigger.c \
  $(SRCDIR)/game/g_utils.c \
  $(SRCDIR)/game/g_weapon.c \
  $(SRCDIR)/game/q_math.c \
  $(SRCDIR)/game/q_shared.c
GAME_OBJ = $(GAME_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(GAME_OBJ): CFLAGS+=-Werror -Wall -DQAGAME

QAGAME_OBJ = $(SQLITE_OBJ) $(HTTPD_OBJ) $(GAME_OBJ)
QAGAME = $(OBJDIR)/qagamei386.so

$(QAGAME): $(QAGAME_OBJ)
	$(ECHO) "LD $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^

QAGAME_DEP = $(QAGAME_OBJ:%.o=%.d)

-include $(QAGAME_DEP)
