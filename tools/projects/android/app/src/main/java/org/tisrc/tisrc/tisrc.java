package org.tisrc.tisrc;
import org.libsdl.app.SDLActivity;

public class tisrc extends SDLActivity {
    protected String getMainFunction() {
        return "main";
    }
    protected String[] getLibraries() {
        return new String[] {
            "SDL2",
            "tisrc"
        };
    }
}
