"""Tests for fb_registry module -- standard IEC 61131-3 function block registry."""

from st2js.fb_registry import FBInfo, get_fb_info, is_standard_fb


class TestFBInfo:
    """Test FBInfo dataclass construction."""

    def test_fbinfo_fields(self):
        info = FBInfo(
            name="TEST",
            js_class="TEST",
            constructor_args=[("PT", "TIME")],
            update_params=[("IN", "BOOL")],
            outputs=[("Q", "BOOL")],
        )
        assert info.name == "TEST"
        assert info.js_class == "TEST"
        assert info.constructor_args == [("PT", "TIME")]
        assert info.update_params == [("IN", "BOOL")]
        assert info.outputs == [("Q", "BOOL")]


class TestGetFBInfo:
    """Test get_fb_info() returns correct FBInfo for all 10 standard FBs."""

    def test_ton(self):
        info = get_fb_info("TON")
        assert info is not None
        assert info.name == "TON"
        assert info.js_class == "TON"
        assert info.constructor_args == [("PT", "TIME")]
        assert info.update_params == [("IN", "BOOL")]
        assert info.outputs == [("Q", "BOOL"), ("ET", "TIME")]

    def test_tof(self):
        info = get_fb_info("TOF")
        assert info is not None
        assert info.name == "TOF"
        assert info.js_class == "TOF"
        assert info.constructor_args == [("PT", "TIME")]
        assert info.update_params == [("IN", "BOOL")]
        assert info.outputs == [("Q", "BOOL"), ("ET", "TIME")]

    def test_tp(self):
        info = get_fb_info("TP")
        assert info is not None
        assert info.name == "TP"
        assert info.js_class == "TP"
        assert info.constructor_args == [("PT", "TIME")]
        assert info.update_params == [("IN", "BOOL")]
        assert info.outputs == [("Q", "BOOL"), ("ET", "TIME")]

    def test_ctu(self):
        info = get_fb_info("CTU")
        assert info is not None
        assert info.name == "CTU"
        assert info.js_class == "CTU"
        assert info.constructor_args == [("PV", "INT")]
        assert info.update_params == [("CU", "BOOL"), ("RESET", "BOOL")]
        assert info.outputs == [("Q", "BOOL"), ("CV", "INT")]

    def test_ctd(self):
        info = get_fb_info("CTD")
        assert info is not None
        assert info.name == "CTD"
        assert info.js_class == "CTD"
        assert info.constructor_args == [("PV", "INT")]
        assert info.update_params == [("CD", "BOOL"), ("LOAD", "BOOL")]
        assert info.outputs == [("Q", "BOOL"), ("CV", "INT")]

    def test_ctud(self):
        info = get_fb_info("CTUD")
        assert info is not None
        assert info.name == "CTUD"
        assert info.js_class == "CTUD"
        assert info.constructor_args == [("PV", "INT")]
        assert info.update_params == [
            ("CU", "BOOL"),
            ("CD", "BOOL"),
            ("RESET", "BOOL"),
            ("LOAD", "BOOL"),
        ]
        assert info.outputs == [("QU", "BOOL"), ("QD", "BOOL"), ("CV", "INT")]

    def test_rs(self):
        info = get_fb_info("RS")
        assert info is not None
        assert info.name == "RS"
        assert info.js_class == "RS"
        assert info.constructor_args == []
        assert info.update_params == [("SET", "BOOL"), ("RESET1", "BOOL")]
        assert info.outputs == [("Q1", "BOOL")]

    def test_sr(self):
        info = get_fb_info("SR")
        assert info is not None
        assert info.name == "SR"
        assert info.js_class == "SR"
        assert info.constructor_args == []
        assert info.update_params == [("SET1", "BOOL"), ("RESET", "BOOL")]
        assert info.outputs == [("Q1", "BOOL")]

    def test_r_trig(self):
        info = get_fb_info("R_TRIG")
        assert info is not None
        assert info.name == "R_TRIG"
        assert info.js_class == "R_TRIG"
        assert info.constructor_args == []
        assert info.update_params == [("CLK", "BOOL")]
        assert info.outputs == [("Q", "BOOL")]

    def test_f_trig(self):
        info = get_fb_info("F_TRIG")
        assert info is not None
        assert info.name == "F_TRIG"
        assert info.js_class == "F_TRIG"
        assert info.constructor_args == []
        assert info.update_params == [("CLK", "BOOL")]
        assert info.outputs == [("Q", "BOOL")]

    def test_unknown_fb_returns_none(self):
        assert get_fb_info("UNKNOWN_FB") is None

    def test_unknown_fb_empty_string_returns_none(self):
        assert get_fb_info("") is None

    def test_case_sensitive(self):
        """FB names are case-sensitive per IEC 61131-3 convention."""
        assert get_fb_info("ton") is None
        assert get_fb_info("Ton") is None


class TestIsStandardFB:
    """Test is_standard_fb() for known and unknown types."""

    def test_all_known_fbs_are_standard(self):
        known = ["TON", "TOF", "TP", "CTU", "CTD", "CTUD", "RS", "SR", "R_TRIG", "F_TRIG"]
        for name in known:
            assert is_standard_fb(name) is True, f"{name} should be standard"

    def test_unknown_fb_is_not_standard(self):
        assert is_standard_fb("UNKNOWN") is False

    def test_empty_string_is_not_standard(self):
        assert is_standard_fb("") is False

    def test_lowercase_is_not_standard(self):
        assert is_standard_fb("ton") is False
