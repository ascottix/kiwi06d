KIWI_OBJECTS = \
    attacks.o \
    bitbase.o \
    bitbase_adjust.o \
    bitbase_generation.o \
    bitboard.o \
    board.o \
    book.o \
    command.o \
    counters.o \
    engine.o \
    engine_main.o \
    engine_perft.o \
    engine_quiesce.o \
    engine_search.o \
    hash.o \
    log.o \
    main.o \
    mask.o \
    move.o \
    movehandler.o \
    movelist.o \
    packed_array.o \
    pawnhash.o \
    pgn.o \
    pgn_lex.o \
    position.o \
    position_domove.o \
    position_enum.o \
    position_evaluate.o \
    position_evaluate_pawn.o \
    position_fen.o \
    position_genmoves.o \
    position_undomove.o \
    random.o \
    recognizer.o \
    recognizer_test.o \
    san.o \
    score.o \
    string.o \
    string_tokenizer.o \
    system.o \
    time_manager.o \
    winboard.o \
    zobrist.o

OBJDIR := ../$(OBJDIR)

OBJECTS = $(addprefix $(OBJDIR)/,$(KIWI_OBJECTS))

LD = g++

KIWI = kiwi
LIBS = -lpthread

CC_FLAGS = -Wall -DEOF_AS_INPUT -DGCC -DLINUX_I386 -O2

target: $(OBJDIR)/$(KIWI)

$(OBJDIR)/$(KIWI): $(OBJECTS)
	$(LD) $(LDFLAGS) $^ $(LIBS) -o $@

$(OBJDIR)/%.o : %.cxx
	$(CC) $(CC_FLAGS) -c $< -o $@
