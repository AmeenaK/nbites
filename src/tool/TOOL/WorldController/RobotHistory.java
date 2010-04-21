package TOOL.WorldController;

import java.util.LinkedList;

public class RobotHistory {

    // These are public becuase they are accesed directly from Field.
    // Its OK, I trust Field.
    // To hold the most recent positions, for each of the robots,
    // for re-drawing each frame
    public int team;
    public int color;
    public int number;
    public LinkedList<LocalizationPacket> actual_position_history =
	new LinkedList<LocalizationPacket>();
    public LinkedList<LocalizationPacket> estimated_position_history =
	new LinkedList<LocalizationPacket>();
    public LinkedList<LocalizationPacket> actual_ball_position_history =
	new LinkedList<LocalizationPacket>();
    public LinkedList<LocalizationPacket> estimated_ball_position_history =
	new LinkedList<LocalizationPacket>();
    // holds set of landmarks the robot uses to localize in the last frame
    public LinkedList<LocalizationPacket> visible_landmarks =
	new LinkedList<LocalizationPacket>();

    public RobotHistory() {
    }
}
