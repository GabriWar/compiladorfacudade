; Codigo Assembly WNEANDER gerado pelo compilador hipotetico

    LOAD 200
    STORE 128
    LOAD 201
    STORE 129
WHILE_0:
    LOAD 128
    JZ ENDWHILE_1
    LOAD 129
    ADD 128
    STORE 129
    LOAD 128
    SUB 202
    STORE 128
    JMP WHILE_0
ENDWHILE_1:
    LOAD 129
    JZ ENDIF_2
    LOAD 129
    STORE 130
ENDIF_2:
    HALT
