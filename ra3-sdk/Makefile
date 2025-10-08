.PHONY: default clean makedirs debug

# ------------------------------------------------------------------------------
# targets
# ------------------------------------------------------------------------------

default: debug

clean:
	rm -rf $(OBJDIR)

makedirs:
	@mkdir -p $(dir $(TOOLS_OBJ))
	@mkdir -p $(dir $(QAGAME_OBJ))

debug: makedirs
	@$(MAKE) CONFIGURATION=debug $(QAGAME)

# ------------------------------------------------------------------------------
# common rules
# ------------------------------------------------------------------------------

SRCDIR=code
OBJDIR=build
BINEXT=
LIBEXT=.so

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

$(OBJDIR)/%.o: $(OBJDIR)/%.c
	$(ECHO) "CC $<"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

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
# tools rules
# ------------------------------------------------------------------------------

LBURG_SRC = \
  $(SRCDIR)/tools/lcc/lburg/lburg.c \
  $(SRCDIR)/tools/lcc/lburg/gram.c

Q3ASM_SRC = \
  $(SRCDIR)/tools/asm/q3asm.c \
  $(SRCDIR)/tools/asm/cmdlib.c

Q3CPP_SRC = \
  $(SRCDIR)/tools/lcc/cpp/cpp.c \
  $(SRCDIR)/tools/lcc/cpp/lex.c \
  $(SRCDIR)/tools/lcc/cpp/nlist.c \
  $(SRCDIR)/tools/lcc/cpp/tokens.c \
  $(SRCDIR)/tools/lcc/cpp/macro.c \
  $(SRCDIR)/tools/lcc/cpp/eval.c \
  $(SRCDIR)/tools/lcc/cpp/include.c \
  $(SRCDIR)/tools/lcc/cpp/hideset.c \
  $(SRCDIR)/tools/lcc/cpp/getopt.c \
  $(SRCDIR)/tools/lcc/cpp/unix.c

Q3LCC_SRC = \
  $(SRCDIR)/tools/lcc/etc/lcc.c \
  $(SRCDIR)/tools/lcc/etc/bytecode.c

Q3RCC_SRC = \
  $(SRCDIR)/tools/lcc/src/alloc.c \
  $(SRCDIR)/tools/lcc/src/bind.c \
  $(SRCDIR)/tools/lcc/src/bytecode.c \
  $(SRCDIR)/tools/lcc/src/dag.c \
  $(SRCDIR)/tools/lcc/src/decl.c \
  $(SRCDIR)/tools/lcc/src/enode.c \
  $(SRCDIR)/tools/lcc/src/error.c \
  $(SRCDIR)/tools/lcc/src/event.c \
  $(SRCDIR)/tools/lcc/src/expr.c \
  $(SRCDIR)/tools/lcc/src/gen.c \
  $(SRCDIR)/tools/lcc/src/init.c \
  $(SRCDIR)/tools/lcc/src/inits.c \
  $(SRCDIR)/tools/lcc/src/input.c \
  $(SRCDIR)/tools/lcc/src/lex.c \
  $(SRCDIR)/tools/lcc/src/list.c \
  $(SRCDIR)/tools/lcc/src/main.c \
  $(SRCDIR)/tools/lcc/src/null.c \
  $(SRCDIR)/tools/lcc/src/output.c \
  $(SRCDIR)/tools/lcc/src/prof.c \
  $(SRCDIR)/tools/lcc/src/profio.c \
  $(SRCDIR)/tools/lcc/src/simp.c \
  $(SRCDIR)/tools/lcc/src/stmt.c \
  $(SRCDIR)/tools/lcc/src/string.c \
  $(SRCDIR)/tools/lcc/src/sym.c \
  $(SRCDIR)/tools/lcc/src/symbolic.c \
  $(SRCDIR)/tools/lcc/src/trace.c \
  $(SRCDIR)/tools/lcc/src/tree.c \
  $(SRCDIR)/tools/lcc/src/types.c

LBURG_OBJ = $(LBURG_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
Q3ASM_OBJ = $(Q3ASM_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
Q3CPP_OBJ = $(Q3CPP_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
Q3LCC_OBJ = $(Q3LCC_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
Q3RCC_OBJ = $(Q3RCC_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o) $(OBJDIR)/tools/lcc/src/dagcheck.o

LBURG = $(OBJDIR)/lburg$(BINEXT)
Q3ASM = $(OBJDIR)/q3asm$(BINEXT)
Q3CPP = $(OBJDIR)/q3cpp$(BINEXT)
Q3LCC = $(OBJDIR)/q3lcc$(BINEXT)
Q3RCC = $(OBJDIR)/q3rcc$(BINEXT)

$(Q3ASM_OBJ): CFLAGS+=-I$(CURDIR)/$(SRCDIR)/tools/lcc/src
$(Q3LCC_OBJ): CFLAGS+=-DTEMPDIR=\"/tmp\"

$(OBJDIR)/tools/lcc/src/dagcheck.o: CFLAGS+=-I$(CURDIR)/$(SRCDIR)/tools/lcc/src

$(OBJDIR)/tools/lcc/src/dagcheck.c: $(SRCDIR)/tools/lcc/src/dagcheck.md $(LBURG)
	$(ECHO) "LBURG $<"
	$(Q)$(LBURG) $< $@

$(LBURG): $(LBURG_OBJ)
	$(ECHO) "LD $@"
	$(Q)$(CC) -o $@ $^

$(Q3ASM): $(Q3ASM_OBJ)
	$(ECHO) "LD $@"
	$(Q)$(CC) -o $@ $^

$(Q3CPP): $(Q3CPP_OBJ)
	$(ECHO) "LD $@"
	$(Q)$(CC) -o $@ $^

$(Q3LCC): $(Q3LCC_OBJ) $(Q3CPP) $(Q3RCC)
	$(ECHO) "LD $@"
	$(Q)$(CC) -o $@ $(Q3LCC_OBJ)

$(Q3RCC): $(Q3RCC_OBJ)
	$(ECHO) "LD $@"
	$(Q)$(CC) -o $@ $^

TOOLS_OBJ = $(LBURG_OBJ) $(Q3ASM_OBJ) $(Q3CPP_OBJ) $(Q3LCC_OBJ) $(Q3RCC_OBJ)
TOOLS_DEP = $(patsubst %.o,%.d,$(TOOLS_OBJ))

-include $(TOOLS_DEP)

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

HTTPD_SRC = \
  $(SRCDIR)/httpd/api.c \
  $(SRCDIR)/httpd/ip_acl.c \
  $(SRCDIR)/httpd/protocol.c

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

ifeq ($(ENABLE_QVM),1)
  GAME_OBJ=$(GAME_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.asm)

  # vmMain has to be the first function in the file
  QAGAME_TMP := $(filter-out $(OBJDIR)/game/g_main.asm $(OBJDIR)/game/g_syscalls.asm,$(GAME_OBJ))
  QAGAME_OBJ = $(OBJDIR)/game/g_main.asm $(QAGAME_TMP)
  QAGAME = $(OBJDIR)/qagame.qvm

  $(OBJDIR)/%.asm: $(SRCDIR)/%.c $(Q3LCC)
		$(ECHO) "CC $<"
		$(Q)$(CC) -DQAGAME -MM -MT $@ -MF $(@:%.asm=%.d) $<
		$(Q)$(Q3LCC) -DQAGAME -o $@ -c $<

  $(QAGAME): $(QAGAME_OBJ) $(SRCDIR)/game/g_syscalls.asm $(Q3ASM)
		$(ECHO) "LD $@"
		$(Q)$(Q3ASM) -v -o $@ $(QAGAME_OBJ) $(SRCDIR)/game/g_syscalls.asm $(QAGAME_LDFLAGS)
else
  SQLITE_OBJ = $(SQLITE_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
  HTTPD_OBJ = $(HTTPD_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
  GAME_OBJ = $(GAME_SRC:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

  $(SQLITE_OBJ): CFLAGS+=-DNO_TCL
  $(GAME_OBJ): CFLAGS+=-Werror -Wall -DQAGAME

  QAGAME_OBJ = $(SQLITE_OBJ) $(HTTPD_OBJ) $(GAME_OBJ)
  QAGAME = $(OBJDIR)/qagamei386.so

  $(QAGAME): $(QAGAME_OBJ)
	$(ECHO) "LD $@"
	$(Q)$(CC) $(LDFLAGS) -o $@ $^
endif

QAGAME_DEP = $(patsubst %.asm,%.d,$(patsubst %.o,%.d,$(QAGAME_OBJ)))
-include $(QAGAME_DEP)
