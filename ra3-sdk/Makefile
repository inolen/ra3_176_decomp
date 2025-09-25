.PHONY: default clean makedirs debug

# ------------------------------------------------------------------------------
# targets
# ------------------------------------------------------------------------------

default: debug

clean:
	rm -rf $(OBJDIR)

makedirs:
	mkdir -p $(dir $(QAGAME_OBJ))

debug: makedirs
	@$(MAKE) CONFIGURATION=debug $(QAGAME)

# ------------------------------------------------------------------------------
# base rules
# ------------------------------------------------------------------------------

SRCDIR = code
OBJDIR = build
BINEXT =
LIBEXT = .so

CFLAGS = -MMD -fPIC -Werror -Wall -DQAGAME
LDFLAGS = -shared -lpthread

ifeq ($(CONFIGURATION),debug)
  CFLAGS += -O0
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
# qagame
# ------------------------------------------------------------------------------

QAGAME_SRC = \
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
  $(SRCDIR)/game/g_captain.c \
  $(SRCDIR)/game/g_client.c \
  $(SRCDIR)/game/g_cmds.c \
  $(SRCDIR)/game/g_combat.c \
  $(SRCDIR)/game/g_db.c \
  $(SRCDIR)/game/g_httpd.c \
  $(SRCDIR)/game/g_items.c \
  $(SRCDIR)/game/g_main.c \
  $(SRCDIR)/game/g_mem.c \
  $(SRCDIR)/game/g_misc.c \
  $(SRCDIR)/game/g_missile.c \
  $(SRCDIR)/game/g_mover.c \
  $(SRCDIR)/game/g_session.c \
  $(SRCDIR)/game/g_spawn.c \
  $(SRCDIR)/game/g_stats.c \
  $(SRCDIR)/game/g_svcmds.c \
  $(SRCDIR)/game/g_syscalls.c \
  $(SRCDIR)/game/g_target.c \
  $(SRCDIR)/game/g_team.c \
  $(SRCDIR)/game/g_thread.c \
  $(SRCDIR)/game/g_trigger.c \
  $(SRCDIR)/game/g_unlagged.c \
  $(SRCDIR)/game/g_utils.c \
  $(SRCDIR)/game/g_weapon.c \
  $(SRCDIR)/game/q_math.c \
  $(SRCDIR)/game/q_shared.c
QAGAME_OBJ = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(QAGAME_SRC))
QAGAME = $(OBJDIR)/qagamei386.so

$(QAGAME): $(QAGAME_OBJ)
	$(ECHO) "LD $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^

QAGAME_DEP = $(patsubst %.o,%.d,$(QAGAME_OBJ))

-include $(QAGAME_DEP)
