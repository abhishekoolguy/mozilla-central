/* -*- Mode: java; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License.  You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is the Grendel mail/news client.
 *
 * The Initial Developer of the Original Code is Netscape Communications
 * Corporation.  Portions created by Netscape are Copyright (C) 1997
 * Netscape Communications Corporation.  All Rights Reserved.
 *
 * Created: Will Scullin <scullin@netscape.com>,  3 Sep 1997.
 */

package grendel.ui;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Insets;
import java.awt.Rectangle;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.beans.PropertyChangeEvent;
import java.beans.PropertyChangeListener;
import java.net.URL;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.MissingResourceException;
import java.util.ResourceBundle;
import java.util.StringTokenizer;
import java.util.Vector;

import com.sun.java.swing.Action;
import com.sun.java.swing.Icon;
import com.sun.java.swing.ImageIcon;
import com.sun.java.swing.JButton;
import com.sun.java.swing.JFrame;
import com.sun.java.swing.JLabel;
import com.sun.java.swing.JMenuBar;
import com.sun.java.swing.JOptionPane;
import com.sun.java.swing.JPanel;
import com.sun.java.swing.JToolBar;
import com.sun.java.swing.SwingUtilities;
import com.sun.java.swing.UIManager;

import netscape.orion.toolbars.BarLayout;
import netscape.orion.toolbars.CollapsibleToolbarPanel;
import netscape.orion.toolbars.NSButton;
import netscape.orion.toolbars.NSToolbar;
import netscape.orion.toolbars.ToolBarLayout;
import netscape.orion.uimanager.AbstractUICmd;
import netscape.orion.uimanager.IUICmd;
import netscape.orion.uimanager.IUIMMenuBar;
import netscape.orion.uimanager.UIMConstants;

import xml.tree.TreeBuilder;
import xml.tree.XMLNode;

import calypso.util.Preferences;
import calypso.util.PreferencesFactory;

import grendel.widgets.Animation;

public class GeneralFrame extends JFrame
{
  GeneralFrame            fThis;

  BiffIcon fBiffIcon;

  protected Container     fPanel;
  protected Animation     fAnimation;
  protected JMenuBar      fMenu;
  protected CollapsibleToolbarPanel fToolBarPanel;
  protected NSToolbar     fToolBar;
  protected Component     fStatusBar;
  protected String        fResourceBase = "grendel.ui";
  protected String        fID;
  protected JLabel        fStatusLabel;

  protected netscape.orion.uimanager.UIManager fUIManager;

  private LAFListener     fLAFListener;

  static Vector fFrameList = new Vector();
  static boolean sExternalShell = false;

  protected ResourceBundle fLabels =
    ResourceBundle.getBundle("grendel.ui.Labels", getLocale());

  public GeneralFrame(String aTitle, String aID) {
    fThis = this;
    fID = aID;

    String title = aTitle;
    try {
      title = fLabels.getString(title);
    } catch (MissingResourceException e) {}

    setTitle(title);

    addWindowListener(new WindowAdapter() {
      public void windowClosing(WindowEvent e) {
        dispose();
      }
    });

    updateUI();
    fLAFListener = new LAFListener();
    UIManager.addPropertyChangeListener(fLAFListener);

    fPanel = getContentPane();

    fAnimation = new Animation();
    fAnimation.setImageTemplate("/grendel/ui/images/animation/AnimHuge{0,number,00}.gif",
                                40);

    fToolBarPanel = new CollapsibleToolbarPanel(this);
    fPanel.add(BorderLayout.NORTH, fToolBarPanel);
    fUIManager = new netscape.orion.uimanager.UIManager(fToolBarPanel);

    // We need to use Class.forName because getClass() might return a child
    // class in another package.

    try {
      URL url = Class.forName("grendel.ui.GeneralFrame").getResource("images/GrendelIcon32.gif");
      setIconImage(getToolkit().getImage(url));
    } catch (ClassNotFoundException e) {
      e.printStackTrace();
    }

    fFrameList.addElement(this);
  }

  public void dispose() {
    if (fBiffIcon != null) {
      fBiffIcon.dispose();
    }

    fFrameList.removeElement(this);

    super.dispose();

    if (!sExternalShell && fFrameList.size() == 0) {
      ActionFactory.GetExitAction().actionPerformed(null);
    }

    UIManager.removePropertyChangeListener(fLAFListener);
  }

  public void updateUI() {
    setBackground(UIManager.getColor("control"));
    getContentPane().setBackground(UIManager.getColor("control"));
  }
  /**
   * Sets the frame's id.
   */

  public void setID(String aID) {
    fID = aID;
  }

  /**
   * Returns the frame's id.
   */

  public String getID() {
    return fID;
  }

  /**
   * Finds the last created frame of a given id.
   */

  public static GeneralFrame FindFrameByID(String aID) {
    return FindFrameByID(aID, null);
  }

  /**
   * Finds the last created frame of a given id.
   */

  public static GeneralFrame FindFrameByID(String aID, GeneralFrame aExclude) {
    for (int i = fFrameList.size() - 1; i >= 0; i--) {
      GeneralFrame frame = (GeneralFrame) fFrameList.elementAt(i);
      if (frame.getID().equals(aID) && frame != aExclude) {
        return frame;
      }
    }
    return null;
  }

  /**
   * Returns a frame for use by dialogs
   */

  static public GeneralFrame GetDefaultFrame() {
    if (fFrameList.size() > 0) {
      return (GeneralFrame) fFrameList.elementAt(0);
    }
    return null;
  }

  /**
   * Sets whether or not this frame is working with an external application.
   * Important to prevent exiting the system when all our frames close.
   */

  static public void SetExternalShell(boolean aShell) {
    sExternalShell = aShell;
  }

  static public boolean IsExternalShell() {
    return sExternalShell;
  }

  static public GeneralFrame[] GetFrameList() {
    GeneralFrame res[] = new GeneralFrame[fFrameList.size()];
    fFrameList.copyInto(res);

    return res;
  }

  static public synchronized void CloseAllFrames() {
    while (fFrameList.size() > 0) {
      GeneralFrame frame = (GeneralFrame) fFrameList.elementAt(0);
      frame.dispose();
    }
  }

  protected JMenuBar buildMenu(String aMenu, IUICmd aActions[]) {
    JMenuBar res = null;
    try {
      IUIMMenuBar bar = fUIManager.getMenuBar(getID());

      URL url = getClass().getResource("menus.xml");
      XMLNode root = TreeBuilder.build(url, getClass());
      XMLNode node = root.getChild(UIMConstants.kMenubarType,
                                   UIMConstants.kIDAttribute,
                                   getID());

      bar.addMenus(node, aActions, this);
      bar.configureForOwner(this);

      res = bar.getComponent();
    } catch (Exception e) {
      e.printStackTrace();
    }

    return res;
  }

  protected Component buildStatusBar() {
    JPanel res = new JPanel(new BarLayout());

    fBiffIcon = new BiffIcon();
    fBiffIcon.setSize(fBiffIcon.getPreferredSize());

    fStatusLabel = new JLabel("Grendel");
    fStatusLabel.setFont(Font.decode("Dialog-12"));

    res.add(fStatusLabel, BarLayout.kSpring);
    res.add(fBiffIcon, BarLayout.kStrut);

    return res;
  }

  protected void startAnimation() {
    fAnimation.start();
  }

  protected void stopAnimation() {
    fAnimation.stop();
  }

  protected void setStatusText(String aString) {
    if (fStatusLabel != null) {
      fStatusLabel.setText(aString);
    }
  }

  private void saveBounds(String aName) {
    Preferences prefs = PreferencesFactory.Get();
    Rectangle bounds = getBounds();

    prefs.putInt(aName + ".x", bounds.x);
    prefs.putInt(aName + ".y", bounds.y);
    prefs.putInt(aName + ".width", bounds.width);
    prefs.putInt(aName + ".height", bounds.height);
  }

  protected void saveBounds() {
    saveBounds(fID);
  }

  private void restoreBounds(String aName, int aWidth, int aHeight) {
    Preferences prefs = PreferencesFactory.Get();
    int x, y, w, h;

    x = prefs.getInt(aName + ".x", 100);
    y = prefs.getInt(aName + ".y", 100);
    w = prefs.getInt(aName + ".width", aWidth);
    h = prefs.getInt(aName + ".height", aHeight);

    setBounds(x, y, w, h);
  }

  protected void restoreBounds(int aWidth, int aHeight) {
    GeneralFrame frame = FindFrameByID(fID, this);
    if (frame == null) {
      restoreBounds(fID, aWidth, aHeight);
    } else {
      Rectangle bounds = frame.getBounds();
      Insets insets = frame.getInsets();
      setBounds(bounds.x + insets.top, bounds.y + insets.top,
                bounds.width, bounds.height);
    }
  }

  protected void restoreBounds() {
    Dimension screenSize = getToolkit().getScreenSize();

    restoreBounds(screenSize.width * 2 / 3,
                  screenSize.height * 2 / 3);
  }

  class LAFListener implements PropertyChangeListener {
    public void propertyChange(PropertyChangeEvent aEvent) {
      SwingUtilities.updateComponentTreeUI(fThis);
      invalidate();
      validate();
      repaint();
    }
  }
}
